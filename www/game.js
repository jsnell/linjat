"use strict";

var cellSize = 25;

class Line {
    constructor(targetLength, r, c) {
        this.targetLength = targetLength;
        this.length = 1;

        this.r = r;
        this.c = c;
        this.h1 = [r, c];
        this.h2 = [r, c];

        this.elem = $("<div>");
        this.elem.addClass("line");

        this.setStyle();
    }

    setStyle() {
        this.setLineStyle(this.h1, this.h2);
    }

    updateAttr(set, klass) {
        if (set) {
            this.elem.addClass(klass);
        } else {
            this.elem.removeClass(klass);
        }
    }

    findHandles(r, c) {
        var h1 = this.h1;
        var h2 = this.h2;
        if (r == h1[0] && c == h1[1]) {
            return [h1, h2];
        }
        if (r == h2[0] && c == h2[1]) {
            return [h2, h1];
        }
    }

    setLineStyle(h1, h2) {
        if (h1[0] > h2[0] || h1[1] > h2[1]) {
            return this.setLineStyle(h2, h1);
        }
        this.elem.css("top", h1[0] * cellSize + 2);
        this.elem.css("left", h1[1] * cellSize + 2);        

        var height = (1 + Math.abs(h1[0] - h2[0]));
        var width = (1 + Math.abs(h1[1] - h2[1]));
        this.elem.css("height", cellSize * height - 4);
        this.elem.css("width", cellSize * width - 4);

        this.length = Math.max(width, height);

        this.updateAttr(this.length == this.targetLength, "line-match");
    }

    findEffectiveHandles(fromR, fromC, r, c) {
        // TODO: Forbid intersecting lines
        var handles = this.findHandles(fromR, fromC);
        if (!handles) {
            if (fromR != r && fromC != c) {
                return null;
            }
            if (fromR == this.r && fromC == this.c) {
                return [[r, c], [fromR, fromC]];
            }
            console.log("should not happen");
            return null;
        }
        if (r != this.r && c != this.c) {
            return null;
        }
        var handle = handles[0];
        var other = handles[1];
        if ((r != other[0]) && (c != other[1])) {
            return [[r, c], [this.r, this.c]];
        } else if ((r < this.r && other[0] < this.r) ||
                   (r > this.r && other[0] > this.r) ||
                   (c < this.c && other[1] < this.c) ||
                   (c > this.c && other[1] > this.c)) {
            return [[this.r, this.c], other];
        } else {
            return [[r, c], other];
        }
        return true;
    }

    updateLine(fromR, fromC, r, c) {
        if (fromR == r && fromR == this.r &&
            fromC == c && fromC == this.c) {
            return true;
        }
        var handles = this.findEffectiveHandles(fromR, fromC, r, c);
        if (handles == null) {
            return false;
        }
        this.setLineStyle(handles[0], handles[1]);
        return true;
    }

    finishUpdate(fromR, fromC, r, c) {
        if (fromR == r && fromR == this.r &&
            fromC == c && fromC == this.c) {
            this.h1 = [r, c];
            this.h2 = [r, c];
            return;
        }
        var handles = this.findEffectiveHandles(fromR, fromC, r, c);
        if (handles == null) {
            return;
        }
        this.h1 = handles[0];
        this.h2 = handles[1];
    }
}

class Cell {
    constructor(r, c, elem) {
        this.r = r;
        this.c = c;
        this.elem = elem;

        elem.css("top", r * cellSize);
        elem.css("left", c * cellSize);
    }

    setStyle() {
        // var line = this.line;
        // if (this.previewLine) {
        //     line = this.previewLine;
        // }
        // this.update(line);
        // if (line) {
        //     if (line.length == line.targetLength) {
        //         this.elem.css("background-color", "#0f0");
        //     } else {
        //         this.elem.css("background-color", "#ff0");
        //     }
        // } else if (this.value == '.') {
        //     this.elem.css("background-color", "red");
        // } else {
        //     this.elem.css("background-color", "");
        // }
        // this.updateAttr(line, 'set');
    }

    update(line) {
        this.updateAttr(this.up && this.up.match(line), 'top-set');
        this.updateAttr(this.down && this.down.match(line), 'bottom-set');
        this.updateAttr(this.left && this.left.match(line), 'left-set');
        this.updateAttr(this.right && this.right.match(line), 'right-set');
    }

    match(line) {
        if (!line) {
            return false;
        }
        return this.line == line || this.previewLine == line;
    }
    
    updateAttr(set, klass) {
        if (set) {
            this.elem.addClass(klass);
        } else {
            this.elem.removeClass(klass);
        }
    }

    setNeighbors(u, d, l, r) {
        this.up = u;
        this.down = d;
        this.left = l;
        this.right = r;
    }
}

class Grid {
    load(rows) {
        var grid = [];
        var lines = [];
        rows.forEach(function (row, ri) {
            var row_array = [];
            grid.push(row_array);
            row.split("").forEach(function (value, ci) {
                var cell = new Cell(ri, ci, $("<div>"));
                cell.value = value;
                if (value > 0 && value <= 9) {
                    lines.push(new Line(value, ri, ci));
                }
                row_array.push(cell);
            });
        });
        this.grid = grid;
        this.lines = lines;
        return grid.length;
    }

    at(r, c) {
        return this.grid[r][c];
    }

    findLineAt(r, c) {
        var ret = null;
        this.eachLine(function(line) {
            if (line.r == r && line.c == c) {
                ret = line;
            } else if (line.h1[0] == r && line.h1[1] == c) {
                ret = line;
            } else if (line.h2[0] == r && line.h2[1] == c) {
                ret = line;
            }
        });
        return ret;
    }

    makeStartDrag(r, c) {
        var grid = this;
        return function(event) {
            var line = grid.findLineAt(r, c);
            if (line) {
                grid.dragLine = line;
                grid.dragStart = [r, c];
            }
            return false;
        };
    }

    makePreviewDrag(r, c) {
        var grid = this;
        return function() {
            if (grid.dragLine) {
                if (!grid.dragLine.updateLine(grid.dragStart[0],
                                              grid.dragStart[1],
                                              r,
                                              c)) {
                    grid.dragLine.setStyle();
                }
                return false;
            }
        };
    }

    makeStopDrag(r, c) {
        var grid = this;
        return function() {
            if (grid.dragLine) {
                if (grid.dragLine.updateLine(grid.dragStart[0],
                                             grid.dragStart[1],
                                             r,
                                             c)) {
                    grid.dragLine.finishUpdate(grid.dragStart[0],
                                               grid.dragStart[1],
                                               r,
                                               c);
                }
                grid.dragLine.setStyle();
                grid.dragLine = null;
            }
            return false;
        };
    }

    makeCancelDrag() {
        var grid = this;
        return function() {
            if (grid.dragLine) {
                grid.dragLine.setStyle();
                grid.dragLine = null;
            }
        }
    }

    updateLine(from, to) {
        if (from[0] == to[0] && from[1] == to[1]) {
            var cell = this.at(from[0], from[1]);
            var line = cell.line;
            if (line && line.owner == cell) {
                this.eachCell(function(c) {
                    if (c.line == line) {
                        c.line = null;
                    }
                });
                cell.line = line;
                line.length = 1;
            } else {
                
            }
            return;
        }
        if (from[0] != to[0] && from[1] != to[1]) {
            console.log("diagonal line");
            return;
        }
        var line = this.at(from[0], from[1]).line;
        if (!line) {
            console.log("no starting point for line");
            return;
        }
        // console.log("extend ", line.owner);
        var dr = (from[0] == to[0] ? 0 : Math.sign(to[0] - from[0]));
        var dc = (from[1] == to[1] ? 0 : Math.sign(to[1] - from[1]));
        while (from[0] != to[0] || from[1] != to[1]) {
            console.log(from);
            from[0] += dr;
            from[1] += dc;
            var cell = this.at(from[0], from[1]);
            if (!cell.line) {
                cell.line = line;
                line.length += 1;
            }
        }
    }

    eachCell(fun) {
        this.grid.forEach(function(row) {
            row.forEach(function(cell) {
                fun(cell);
            })
        });
    }

    eachLine(fun) {
        this.lines.forEach(function(line) {
            fun(line);
        });
    }

    updateStyles() {
        this.eachCell(function(cell) {
            cell.setStyle();
        });
    }    
}

function init() {
    var grid = new Grid();
    var size = grid.load([
"    5 4  2 3",
"  . 5 ..5.  ",
"  3.. .  3  ",
"   22    . 4",
"  .   4 ..  ",
"  3         ",
"     2.  5  ",
" .2  .     .",
".   .  4  23",
"     3...53 ",
"3  52. 32   ",
"  ..4. .    ",
    ]);
    var board = $("#board");
    board.css("width", cellSize * size);
    grid.eachLine(function (line) {
        board.append(line.elem);
    });
    grid.eachCell(function (cell) {
        board.append(cell.elem);
        cell.elem.text(cell.value);
        var r = cell.r;
        var c = cell.c;
        cell.elem.mousedown(grid.makeStartDrag(r, c));
        cell.elem.mouseup(grid.makeStopDrag(r, c));
        cell.elem.mousemove(grid.makePreviewDrag(r, c));
    });
    board.mouseleave(grid.makeCancelDrag(board));

    board.css("width", cellSize * size + 3);
    board.css("height", cellSize * size + 3);

    grid.updateStyles();

    $("body").mousedown(function() { return false });

    var scale = Math.min(($(window).innerHeight() - 64) / board.height(),
                         ($(window).innerWidth() - 64) / board.width());
    board.css("transform", "scale(" + scale + ")");
    board.css("transform-origin", "0 0");
}

