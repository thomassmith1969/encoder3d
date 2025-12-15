/**
 * SVGLoader for Three.js
 * Loads SVG files and converts paths to Three.js shapes
 */

THREE.SVGLoader = function(manager) {
    this.manager = (manager !== undefined) ? manager : THREE.DefaultLoadingManager;
};

THREE.SVGLoader.prototype = {
    constructor: THREE.SVGLoader,

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

    parse: function(text) {
        var parser = new DOMParser();
        var doc = parser.parseFromString(text, 'image/svg+xml');
        
        var paths = [];
        var svg = doc.querySelector('svg');
        
        if (!svg) return { paths: paths };

        function getStyle(node) {
            var style = {};
            var cssText = node.getAttribute('style');
            
            if (cssText) {
                var styles = cssText.split(';');
                for (var i = 0; i < styles.length; i++) {
                    var parts = styles[i].split(':');
                    if (parts.length === 2) {
                        style[parts[0].trim()] = parts[1].trim();
                    }
                }
            }
            
            return style;
        }

        function parsePathNode(node) {
            var d = node.getAttribute('d');
            if (!d) return null;

            var style = getStyle(node);
            var fill = node.getAttribute('fill') || style.fill || '#000000';
            var stroke = node.getAttribute('stroke') || style.stroke;
            var strokeWidth = parseFloat(node.getAttribute('stroke-width') || style['stroke-width'] || 1);

            return {
                d: d,
                fill: fill,
                stroke: stroke,
                strokeWidth: strokeWidth
            };
        }

        // Parse all path elements
        var pathElements = doc.querySelectorAll('path');
        for (var i = 0; i < pathElements.length; i++) {
            var pathData = parsePathNode(pathElements[i]);
            if (pathData) {
                paths.push(pathData);
            }
        }

        // Parse circles
        var circles = doc.querySelectorAll('circle');
        for (var i = 0; i < circles.length; i++) {
            var cx = parseFloat(circles[i].getAttribute('cx') || 0);
            var cy = parseFloat(circles[i].getAttribute('cy') || 0);
            var r = parseFloat(circles[i].getAttribute('r') || 0);
            
            var style = getStyle(circles[i]);
            var fill = circles[i].getAttribute('fill') || style.fill || '#000000';
            
            paths.push({
                type: 'circle',
                cx: cx,
                cy: cy,
                r: r,
                fill: fill
            });
        }

        // Parse rectangles
        var rects = doc.querySelectorAll('rect');
        for (var i = 0; i < rects.length; i++) {
            var x = parseFloat(rects[i].getAttribute('x') || 0);
            var y = parseFloat(rects[i].getAttribute('y') || 0);
            var width = parseFloat(rects[i].getAttribute('width') || 0);
            var height = parseFloat(rects[i].getAttribute('height') || 0);
            
            var style = getStyle(rects[i]);
            var fill = rects[i].getAttribute('fill') || style.fill || '#000000';
            
            paths.push({
                type: 'rect',
                x: x,
                y: y,
                width: width,
                height: height,
                fill: fill
            });
        }

        return { paths: paths };
    },

    // Create Three.js shapes from parsed SVG data
    createShapes: function(data, extrudeDepth) {
        extrudeDepth = extrudeDepth || 1;
        var shapes = [];
        
        for (var i = 0; i < data.paths.length; i++) {
            var pathData = data.paths[i];
            
            if (pathData.type === 'circle') {
                var shape = new THREE.Shape();
                shape.absarc(pathData.cx, -pathData.cy, pathData.r, 0, Math.PI * 2, false);
                shapes.push({ shape: shape, color: pathData.fill });
            } else if (pathData.type === 'rect') {
                var shape = new THREE.Shape();
                shape.moveTo(pathData.x, -pathData.y);
                shape.lineTo(pathData.x + pathData.width, -pathData.y);
                shape.lineTo(pathData.x + pathData.width, -(pathData.y + pathData.height));
                shape.lineTo(pathData.x, -(pathData.y + pathData.height));
                shape.lineTo(pathData.x, -pathData.y);
                shapes.push({ shape: shape, color: pathData.fill });
            }
        }
        
        return shapes;
    }
};
