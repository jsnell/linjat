"use strict";

class Line {
    constructor(cell) {
        this.owner = cell;
        this.targetLength = cell.value;
        this.length = 1;
    }
}

class Cell {
    setStyle() {
        var line = this.line;
        if (this.previewLine) {
            line = this.previewLine;
        }
        this.update(line);
        if (line) {
            if (line.length == line.targetLength) {
                this.elem.css("background-color", "#0f0");
            } else {
                this.elem.css("background-color", "#ff0");
            }
        } else if (this.value == '.') {
            this.elem.css("background-color", "red");
        } else {
            this.elem.css("background-color", "");
        }
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
        rows.forEach(function (row) {
            var row_array = [];
            grid.push(row_array);
            row.split("").forEach(function (value) {
                var cell = new Cell();
                cell.value = value;
                if (value > 0 && value <= 9) {
                    cell.line = new Line(cell);
                }
                row_array.push(cell);
            });
        });
        this.grid = grid;
        return grid.length;
    }

    at(r, c) {
        return this.grid[r][c];
    }

    makeStartDrag(r, c) {
        var grid = this;
        return function() {
            var line = grid.at(r, c).line;
            if (line) {
                grid.previewLine = new Line(line.owner);
                grid.drag = [r, c];
                grid.eachCell(function(cell) {
                    if (cell.line == line) {
                        cell.previewLine = grid.previewLine;
                        grid.previewLine.length += 1;
                    } else {
                        cell.previewLine = null;
                    }
                });
                grid.at(r, c).previewLine.length -= 1;
            }
            return false;
        };
    }

    makeStopDrag(r, c) {
        var grid = this;
        return function() {
            if (grid.previewLine) {
                grid.eachCell(function(cell) {
                    cell.previewLine = null;
                });
                grid.previewLine = null;
                grid.updateLine(grid.drag, [r, c]);
                grid.updateStyles();
            }
            return false;
        };
    }

    makePreviewDrag(r, c) {
        var grid = this;
        return function() {
            if (grid.previewLine) {
                if (grid.at(r, c).previewLine != grid.previewLine) {
                    grid.at(r, c).previewLine = grid.previewLine;
                    grid.previewLine.length += 1;
                    grid.updateStyles();
                }
                return false;
            }
        };
    }

    makeCancelDrag() {
        var grid = this;
        return function() {
            if (grid.previewLine) {
                grid.eachCell(function(cell) {
                    cell.previewLine = null;
                });
                grid.previewLine = null;
                grid.updateStyles();
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

    updateStyles() {
        this.eachCell(function(cell) {
            cell.setStyle();
        });
    }    
}

function init() {
    var grid = new Grid();
    var size = grid.load([
" 2 4  2.2   ",
"  2  2. .3 .",
"   4.2 . 3 5",
"4 4..4 ..5. ",
"   .  2     ",
"3  4        ",
"    4.. 4.4.",
"2           ",
".   . 35  23",
"    .  4. . ",
"        2.2.",
"242.      .3",
    ]);
    var board = $("#board");
    for (var r = 0; r < size; ++r) {
        var tr = $("<tr>");
        for (var c = 0; c < size; ++c) {
            var td = $("<td>");
            td.text(grid.at(r, c).value);
            td.mousedown(grid.makeStartDrag(r, c));
            td.mousemove(grid.makePreviewDrag(r, c));
            td.mouseup(grid.makeStopDrag(r, c));
            tr.append(td);
            grid.at(r, c).elem = td;
        }
        board.append(tr);
    }
    for (var r = 0; r < size; ++r) {
        for (var c = 0; c < size; ++c) {
            var up = (r == 0 ? null : grid.at(r-1, c));
            var down = (r == size - 1 ? null : grid.at(r+1, c));
            var left = (c == 0 ? null : grid.at(r, c-1));
            var right = (c == size - 1 ? null : grid.at(r ,c+1));
            grid.at(r, c).setNeighbors(up, down, left, right);
        }
    }
    board.mouseleave(grid.makeCancelDrag());

    grid.updateStyles();

    $("body").mousedown(function() { return false });
}

