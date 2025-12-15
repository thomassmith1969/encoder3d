/**
 * OBJLoader for Three.js
 * Loads Wavefront .obj files
 */

THREE.OBJLoader = function(manager) {
    this.manager = (manager !== undefined) ? manager : THREE.DefaultLoadingManager;
    this.materials = null;
};

THREE.OBJLoader.prototype = {
    constructor: THREE.OBJLoader,

    load: function(url, onLoad, onProgress, onError) {
        var scope = this;
        var loader = new THREE.FileLoader(scope.manager);
        loader.setPath(this.path);
        loader.load(url, function(text) {
            onLoad(scope.parse(text));
        }, onProgress, onError);
    },

    setPath: function(value) {
        this.path = value;
        return this;
    },

    setMaterials: function(materials) {
        this.materials = materials;
        return this;
    },

    parse: function(text) {
        var object, objects = [];
        var geometry, material;

        function parseVertexIndex(value) {
            var index = parseInt(value);
            return (index >= 0 ? index - 1 : index + vertices.length / 3) * 3;
        }

        function parseNormalIndex(value) {
            var index = parseInt(value);
            return (index >= 0 ? index - 1 : index + normals.length / 3) * 3;
        }

        function parseUVIndex(value) {
            var index = parseInt(value);
            return (index >= 0 ? index - 1 : index + uvs.length / 2) * 2;
        }

        function addVertex(a, b, c) {
            geometry.vertices.push(
                vertices[a], vertices[a + 1], vertices[a + 2],
                vertices[b], vertices[b + 1], vertices[b + 2],
                vertices[c], vertices[c + 1], vertices[c + 2]
            );
        }

        function addNormal(a, b, c) {
            geometry.normals.push(
                normals[a], normals[a + 1], normals[a + 2],
                normals[b], normals[b + 1], normals[b + 2],
                normals[c], normals[c + 1], normals[c + 2]
            );
        }

        function addUV(a, b, c) {
            geometry.uvs.push(
                uvs[a], uvs[a + 1],
                uvs[b], uvs[b + 1],
                uvs[c], uvs[c + 1]
            );
        }

        function addFace(a, b, c, ua, ub, uc, na, nb, nc) {
            var ia = parseVertexIndex(a);
            var ib = parseVertexIndex(b);
            var ic = parseVertexIndex(c);

            addVertex(ia, ib, ic);

            if (ua !== undefined && ub !== undefined && uc !== undefined) {
                addUV(parseUVIndex(ua), parseUVIndex(ub), parseUVIndex(uc));
            }

            if (na !== undefined && nb !== undefined && nc !== undefined) {
                addNormal(parseNormalIndex(na), parseNormalIndex(nb), parseNormalIndex(nc));
            }
        }

        var vertices = [];
        var normals = [];
        var uvs = [];

        function parseObject(name) {
            var geometry = {
                vertices: [],
                normals: [],
                uvs: []
            };

            var material = {
                name: '',
                smooth: false
            };

            return {
                name: name,
                geometry: geometry,
                material: material
            };
        }

        var object = parseObject('');
        objects.push(object);
        geometry = object.geometry;
        material = object.material;

        var lines = text.split('\n');

        for (var i = 0; i < lines.length; i++) {
            var line = lines[i].trim();

            if (line.length === 0 || line.charAt(0) === '#') {
                continue;
            }

            var result = line.split(/\s+/);
            var command = result.shift().toLowerCase();

            if (command === 'v') {
                vertices.push(
                    parseFloat(result[0]),
                    parseFloat(result[1]),
                    parseFloat(result[2])
                );
            } else if (command === 'vn') {
                normals.push(
                    parseFloat(result[0]),
                    parseFloat(result[1]),
                    parseFloat(result[2])
                );
            } else if (command === 'vt') {
                uvs.push(
                    parseFloat(result[0]),
                    parseFloat(result[1])
                );
            } else if (command === 'f') {
                var faces = [];
                
                for (var j = 0; j < result.length; j++) {
                    var vertex = result[j];
                    if (vertex.length > 0) {
                        var vertexParts = vertex.split('/');
                        faces.push({
                            v: vertexParts[0],
                            vt: vertexParts[1],
                            vn: vertexParts[2]
                        });
                    }
                }

                var v1 = faces[0];
                for (var j = 1; j < faces.length - 1; j++) {
                    var v2 = faces[j];
                    var v3 = faces[j + 1];
                    addFace(
                        v1.v, v2.v, v3.v,
                        v1.vt, v2.vt, v3.vt,
                        v1.vn, v2.vn, v3.vn
                    );
                }
            } else if (command === 'o' || command === 'g') {
                object = parseObject(result[0]);
                objects.push(object);
                geometry = object.geometry;
                material = object.material;
            }
        }

        var container = new THREE.Group();

        for (var i = 0; i < objects.length; i++) {
            object = objects[i];
            geometry = object.geometry;

            var buffergeometry = new THREE.BufferGeometry();

            buffergeometry.setAttribute('position', new THREE.Float32BufferAttribute(geometry.vertices, 3));

            if (geometry.normals.length > 0) {
                buffergeometry.setAttribute('normal', new THREE.Float32BufferAttribute(geometry.normals, 3));
            } else {
                buffergeometry.computeVertexNormals();
            }

            if (geometry.uvs.length > 0) {
                buffergeometry.setAttribute('uv', new THREE.Float32BufferAttribute(geometry.uvs, 2));
            }

            var mat = new THREE.MeshPhongMaterial({ color: 0x888888 });
            var mesh = new THREE.Mesh(buffergeometry, mat);
            mesh.name = object.name;

            container.add(mesh);
        }

        return container.children.length === 1 ? container.children[0] : container;
    }
};
