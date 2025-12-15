/**
 * PLYLoader for Three.js
 * Loads Polygon File Format / Stanford Triangle Format files
 */

THREE.PLYLoader = function(manager) {
    this.manager = (manager !== undefined) ? manager : THREE.DefaultLoadingManager;
    this.propertyNameMapping = {};
};

THREE.PLYLoader.prototype = {
    constructor: THREE.PLYLoader,

    load: function(url, onLoad, onProgress, onError) {
        var scope = this;
        var loader = new THREE.FileLoader(scope.manager);
        loader.setPath(this.path);
        loader.setResponseType('arraybuffer');
        loader.load(url, function(data) {
            onLoad(scope.parse(data));
        }, onProgress, onError);
    },

    setPath: function(value) {
        this.path = value;
        return this;
    },

    parse: function(data) {
        function parseHeader(data) {
            var patternHeader = /ply([\s\S]*)end_header\r?\n/;
            var headerText = '';
            var headerLength = 0;
            var result = patternHeader.exec(data);

            if (result !== null) {
                headerText = result[1];
                headerLength = result[0].length;
            }

            var header = {
                comments: [],
                elements: [],
                headerLength: headerLength
            };

            var lines = headerText.split('\n');
            var currentElement;

            for (var i = 0; i < lines.length; i++) {
                var line = lines[i].trim();
                if (line === '') continue;

                var lineValues = line.split(/\s+/);
                var lineType = lineValues.shift();
                line = lineValues.join(' ');

                if (lineType === 'format') {
                    header.format = lineValues[0];
                    header.version = lineValues[1];
                } else if (lineType === 'comment') {
                    header.comments.push(line);
                } else if (lineType === 'element') {
                    if (currentElement !== undefined) {
                        header.elements.push(currentElement);
                    }
                    currentElement = {
                        name: lineValues[0],
                        count: parseInt(lineValues[1]),
                        properties: []
                    };
                } else if (lineType === 'property') {
                    currentElement.properties.push({
                        type: lineValues[0],
                        name: lineValues[1]
                    });
                }
            }

            if (currentElement !== undefined) {
                header.elements.push(currentElement);
            }

            return header;
        }

        function parseASCIINumber(n, type) {
            switch (type) {
                case 'char': case 'uchar': case 'short': case 'ushort': case 'int': case 'uint':
                case 'int8': case 'uint8': case 'int16': case 'uint16': case 'int32': case 'uint32':
                    return parseInt(n);
                case 'float': case 'double': case 'float32': case 'float64':
                    return parseFloat(n);
            }
        }

        function parseASCII(data, header) {
            var buffer = {
                indices: [],
                vertices: [],
                normals: [],
                uvs: [],
                colors: []
            };

            var patternBody = /end_header\s+/;
            var body = data.slice(data.search(patternBody) + 11);
            var lines = body.split('\n');
            var currentElement = 0;
            var currentElementCount = 0;

            for (var i = 0; i < lines.length; i++) {
                var line = lines[i].trim();
                if (line === '') continue;

                var lineValues = line.split(/\s+/);
                var element = header.elements[currentElement];
                var properties = element.properties;

                if (element.name === 'vertex') {
                    for (var j = 0; j < properties.length; j++) {
                        var property = properties[j];
                        var value = parseASCIINumber(lineValues[j], property.type);

                        if (property.name === 'x') buffer.vertices.push(value);
                        else if (property.name === 'y') buffer.vertices.push(value);
                        else if (property.name === 'z') buffer.vertices.push(value);
                        else if (property.name === 'nx') buffer.normals.push(value);
                        else if (property.name === 'ny') buffer.normals.push(value);
                        else if (property.name === 'nz') buffer.normals.push(value);
                        else if (property.name === 's') buffer.uvs.push(value);
                        else if (property.name === 't') buffer.uvs.push(value);
                        else if (property.name === 'red') buffer.colors.push(value / 255);
                        else if (property.name === 'green') buffer.colors.push(value / 255);
                        else if (property.name === 'blue') buffer.colors.push(value / 255);
                    }
                } else if (element.name === 'face') {
                    var count = parseASCIINumber(lineValues[0], 'int');
                    var indices = [];

                    for (var j = 1; j <= count; j++) {
                        indices.push(parseASCIINumber(lineValues[j], 'int'));
                    }

                    for (var j = 1; j < count - 1; j++) {
                        buffer.indices.push(indices[0], indices[j], indices[j + 1]);
                    }
                }

                currentElementCount++;
                if (currentElementCount >= element.count) {
                    currentElement++;
                    currentElementCount = 0;
                }
            }

            return buffer;
        }

        function parseBinary(data, header) {
            var buffer = {
                indices: [],
                vertices: [],
                normals: [],
                uvs: [],
                colors: []
            };

            var view = new DataView(data, header.headerLength);
            var offset = 0;
            var littleEndian = (header.format === 'binary_little_endian');

            for (var i = 0; i < header.elements.length; i++) {
                var element = header.elements[i];

                for (var j = 0; j < element.count; j++) {
                    for (var k = 0; k < element.properties.length; k++) {
                        var property = element.properties[k];
                        var value;

                        if (property.type === 'float' || property.type === 'float32') {
                            value = view.getFloat32(offset, littleEndian);
                            offset += 4;
                        } else if (property.type === 'int' || property.type === 'int32') {
                            value = view.getInt32(offset, littleEndian);
                            offset += 4;
                        } else if (property.type === 'uchar' || property.type === 'uint8') {
                            value = view.getUint8(offset);
                            offset += 1;
                        }

                        if (element.name === 'vertex') {
                            if (property.name === 'x') buffer.vertices.push(value);
                            else if (property.name === 'y') buffer.vertices.push(value);
                            else if (property.name === 'z') buffer.vertices.push(value);
                            else if (property.name === 'nx') buffer.normals.push(value);
                            else if (property.name === 'ny') buffer.normals.push(value);
                            else if (property.name === 'nz') buffer.normals.push(value);
                            else if (property.name === 'red') buffer.colors.push(value / 255);
                            else if (property.name === 'green') buffer.colors.push(value / 255);
                            else if (property.name === 'blue') buffer.colors.push(value / 255);
                        } else if (element.name === 'face') {
                            if (k === 0) {
                                var count = value;
                                var indices = [];

                                for (var l = 0; l < count; l++) {
                                    indices.push(view.getInt32(offset, littleEndian));
                                    offset += 4;
                                }

                                for (var l = 1; l < count - 1; l++) {
                                    buffer.indices.push(indices[0], indices[l], indices[l + 1]);
                                }
                            }
                        }
                    }
                }
            }

            return buffer;
        }

        var geometry = new THREE.BufferGeometry();
        var header;

        if (data instanceof ArrayBuffer) {
            var text = THREE.LoaderUtils.decodeText(new Uint8Array(data));
            header = parseHeader(text);

            if (header.format === 'ascii') {
                var buffer = parseASCII(text, header);
            } else {
                var buffer = parseBinary(data, header);
            }
        } else {
            header = parseHeader(data);
            var buffer = parseASCII(data, header);
        }

        if (buffer.indices.length > 0) {
            geometry.setIndex(buffer.indices);
        }

        geometry.setAttribute('position', new THREE.Float32BufferAttribute(buffer.vertices, 3));

        if (buffer.normals.length > 0) {
            geometry.setAttribute('normal', new THREE.Float32BufferAttribute(buffer.normals, 3));
        }

        if (buffer.uvs.length > 0) {
            geometry.setAttribute('uv', new THREE.Float32BufferAttribute(buffer.uvs, 2));
        }

        if (buffer.colors.length > 0) {
            geometry.setAttribute('color', new THREE.Float32BufferAttribute(buffer.colors, 3));
        }

        geometry.computeBoundingSphere();

        return geometry;
    }
};
