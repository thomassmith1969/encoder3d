/**
 * STLLoader for Three.js
 * Loads binary and ASCII STL files
 */

THREE.STLLoader = function(manager) {
    this.manager = (manager !== undefined) ? manager : THREE.DefaultLoadingManager;
};

THREE.STLLoader.prototype = {
    constructor: THREE.STLLoader,

    load: function(url, onLoad, onProgress, onError) {
        var scope = this;
        var loader = new THREE.FileLoader(scope.manager);
        loader.setPath(this.path);
        loader.setResponseType('arraybuffer');
        loader.load(url, function(data) {
            try {
                onLoad(scope.parse(data));
            } catch (e) {
                if (onError) {
                    onError(e);
                } else {
                    console.error(e);
                }
                scope.manager.itemError(url);
            }
        }, onProgress, onError);
    },

    setPath: function(value) {
        this.path = value;
        return this;
    },

    parse: function(data) {
        function isBinary(data) {
            var reader = new DataView(data);
            var numFaces = reader.getUint32(80, true);
            var faceSize = (32 / 8 * 3) + ((32 / 8 * 3) * 3) + (16 / 8);
            var expectedBinarySize = 80 + 4 + (numFaces * faceSize);
            if (expectedBinarySize === reader.byteLength) {
                return true;
            }

            var solid = [115, 111, 108, 105, 100];
            for (var i = 0; i < 5; i++) {
                if (solid[i] !== reader.getUint8(i)) return true;
            }
            return false;
        }

        function parseBinary(data) {
            var reader = new DataView(data);
            var faces = reader.getUint32(80, true);
            var dataOffset = 84;
            var faceLength = 12 * 4 + 2;

            var geometry = new THREE.BufferGeometry();
            var vertices = new Float32Array(faces * 3 * 3);
            var normals = new Float32Array(faces * 3 * 3);

            for (var face = 0; face < faces; face++) {
                var start = dataOffset + face * faceLength;
                var normalX = reader.getFloat32(start, true);
                var normalY = reader.getFloat32(start + 4, true);
                var normalZ = reader.getFloat32(start + 8, true);

                for (var i = 1; i <= 3; i++) {
                    var vertexstart = start + i * 12;
                    var index = (face * 3 * 3) + ((i - 1) * 3);

                    vertices[index] = reader.getFloat32(vertexstart, true);
                    vertices[index + 1] = reader.getFloat32(vertexstart + 4, true);
                    vertices[index + 2] = reader.getFloat32(vertexstart + 8, true);

                    normals[index] = normalX;
                    normals[index + 1] = normalY;
                    normals[index + 2] = normalZ;
                }
            }

            geometry.setAttribute('position', new THREE.BufferAttribute(vertices, 3));
            geometry.setAttribute('normal', new THREE.BufferAttribute(normals, 3));

            return geometry;
        }

        function parseASCII(data) {
            var geometry = new THREE.BufferGeometry();
            var patternFace = /facet([\s\S]*?)endfacet/g;
            var faceCounter = 0;

            var text = ensureString(data);
            var result;

            while ((result = patternFace.exec(text)) !== null) {
                faceCounter++;
            }

            var vertices = new Float32Array(faceCounter * 3 * 3);
            var normals = new Float32Array(faceCounter * 3 * 3);

            var patternNormal = /normal\s+([\-+]?[0-9]+\.?[0-9]*([eE][\-+]?[0-9]+)?)\s+([\-+]?[0-9]+\.?[0-9]*([eE][\-+]?[0-9]+)?)\s+([\-+]?[0-9]+\.?[0-9]*([eE][\-+]?[0-9]+)?)/g;
            var patternVertex = /vertex\s+([\-+]?[0-9]+\.?[0-9]*([eE][\-+]?[0-9]+)?)\s+([\-+]?[0-9]+\.?[0-9]*([eE][\-+]?[0-9]+)?)\s+([\-+]?[0-9]+\.?[0-9]*([eE][\-+]?[0-9]+)?)/g;

            patternFace.lastIndex = 0;
            var faceIndex = 0;

            while ((result = patternFace.exec(text)) !== null) {
                var vertexCountPerFace = 0;
                var normalCountPerFace = 0;

                var vertexMatches = result[1].match(patternVertex);
                var normalMatches = result[1].match(patternNormal);

                if (normalMatches) {
                    var normalMatch = normalMatches[0].match(/[\-+]?[0-9]+\.?[0-9]*([eE][\-+]?[0-9]+)?/g);
                    var normalX = parseFloat(normalMatch[0]);
                    var normalY = parseFloat(normalMatch[1]);
                    var normalZ = parseFloat(normalMatch[2]);
                }

                if (vertexMatches) {
                    for (var i = 0; i < vertexMatches.length; i++) {
                        var vertexMatch = vertexMatches[i].match(/[\-+]?[0-9]+\.?[0-9]*([eE][\-+]?[0-9]+)?/g);
                        var x = parseFloat(vertexMatch[0]);
                        var y = parseFloat(vertexMatch[1]);
                        var z = parseFloat(vertexMatch[2]);

                        var index = faceIndex * 9 + vertexCountPerFace * 3;
                        vertices[index] = x;
                        vertices[index + 1] = y;
                        vertices[index + 2] = z;

                        normals[index] = normalX;
                        normals[index + 1] = normalY;
                        normals[index + 2] = normalZ;

                        vertexCountPerFace++;
                    }
                }

                faceIndex++;
            }

            geometry.setAttribute('position', new THREE.BufferAttribute(vertices, 3));
            geometry.setAttribute('normal', new THREE.BufferAttribute(normals, 3));

            return geometry;
        }

        function ensureString(buffer) {
            if (typeof buffer !== 'string') {
                return THREE.LoaderUtils.decodeText(new Uint8Array(buffer));
            }
            return buffer;
        }

        function ensureBinary(buffer) {
            if (typeof buffer === 'string') {
                var array_buffer = new Uint8Array(buffer.length);
                for (var i = 0; i < buffer.length; i++) {
                    array_buffer[i] = buffer.charCodeAt(i) & 0xff;
                }
                return array_buffer.buffer || array_buffer;
            } else {
                return buffer;
            }
        }

        var binData = ensureBinary(data);
        return isBinary(binData) ? parseBinary(binData) : parseASCII(ensureString(data));
    }
};
