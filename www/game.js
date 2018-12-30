"use strict";

var cellSize = 50;

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

        this.dragLeftStartSquare = false;
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
        this.elem.css("top", h1[0] * cellSize + 4);
        this.elem.css("left", h1[1] * cellSize + 4);        

        var height = (1 + Math.abs(h1[0] - h2[0]));
        var width = (1 + Math.abs(h1[1] - h2[1]));
        this.elem.css("height", cellSize * height - 8);
        this.elem.css("width", cellSize * width - 8);

        this.length = Math.max(width, height);

        this.updateAttr(this.length == this.targetLength, "line-match");
        this.updateAttr(this.length > this.targetLength, "line-toolong");
    }

    intersects(h1, h2) {
        var a_min_r = Math.min(this.h1[0], this.h2[0]);
        var a_max_r = Math.max(this.h1[0], this.h2[0]);
        var a_min_c = Math.min(this.h1[1], this.h2[1]);
        var a_max_c = Math.max(this.h1[1], this.h2[1]);
        var b_min_r = Math.min(h1[0], h2[0]);
        var b_max_r = Math.max(h1[0], h2[0]);
        var b_min_c = Math.min(h1[1], h2[1]);
        var b_max_c = Math.max(h1[1], h2[1]);
        var r_intersect =
            (a_min_r >= b_min_r && a_min_r <= b_max_r) ||
            (b_min_r >= a_min_r && b_min_r <= a_max_r);
        var c_intersect =
            (a_min_c >= b_min_c && a_min_c <= b_max_c) ||
            (b_min_c >= a_min_c && b_min_c <= a_max_c);

        return r_intersect && c_intersect;
    }

    findEffectiveHandles(fromR, fromC, r, c, lines) {
        if (fromR == this.r && fromC == this.c &&
            !this.dragLeftStartSquare) {
            return [[fromR, fromC], [fromR, fromC]];
        }
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
        var ret;
        if ((r != other[0]) && (c != other[1])) {
            ret = [[r, c], [this.r, this.c]];
        } else if ((r < this.r && other[0] < this.r) ||
                   (r > this.r && other[0] > this.r) ||
                   (c < this.c && other[1] < this.c) ||
                   (c > this.c && other[1] > this.c)) {
            ret = [[this.r, this.c], other];
        } else {
            ret = [[r, c], other];
        }

        var self = this;
        lines.forEach(function (line) {
            if (line == self)
                return;
            while (true) {
                var intersect = line.intersects(ret[0], [self.r, self.c]);
                if (!intersect)
                    break;
                self.moveTowardCenter(ret[0]);
            }
            while (true) {
                var intersect = line.intersects(ret[1], [self.r, self.c]);
                if (!intersect)
                    break;
                self.moveTowardCenter(ret[1]);
            }
        });

        return ret;
    }

    moveTowardCenter(handle) {
        if (handle[0] < this.r)
            handle[0]++;
        if (handle[0] > this.r)
            handle[0]--;
        if (handle[1] < this.c)
            handle[1]++;
        if (handle[1] > this.c)
            handle[1]--;
    }

    updateLine(fromR, fromC, r, c, lines) {
        if (fromR == r && fromR == this.r &&
            fromC == c && fromC == this.c &&
            !this.dragLeftStartSquare) {
            return true;
        }
        var handles = this.findEffectiveHandles(fromR, fromC, r, c, lines);
        if (handles == null) {
            return false;
        }
        this.setLineStyle(handles[0], handles[1]);
        return true;
    }

    finishUpdate(fromR, fromC, r, c, lines) {
        var handles = this.findEffectiveHandles(fromR, fromC, r, c, lines);
        if (handles == null) {
            return;
        }
        this.h1 = handles[0];
        this.h2 = handles[1];
    }

    reset() {
        this.h1 = [this.r, this.c];
        this.h2 = [this.r, this.c];
        this.setStyle();
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
                var elem = $("<div>");
                var cell = new Cell(ri, ci, elem);
                cell.value = value;
                elem.data('cell', cell);
                if (value > 0 && value <= 9) {
                    lines.push(new Line(value, ri, ci));
                }
                row_array.push(cell);
            });
        });
        this.grid = grid;
        this.lines = lines;
        this.dragStart = null;
        this.dragLine = null;

        return { width: grid[0].length, height: grid.length };
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

    startDrag(r, c) {
        var line = this.findLineAt(r, c);
        if (line) {
            this.dragLine = line;
            line.dragLeftStartSquare = false;
            this.dragStart = [r, c];
        }
    }

    previewDrag(r, c) {
        if (!this.dragLine)
            return;
        if (r != this.dragStart[0] || c != this.dragStart[1])
            this.dragLine.dragLeftStartSquare = true;
        if (!this.dragLine.updateLine(this.dragStart[0],
                                      this.dragStart[1],
                                      r,
                                      c,
                                      this.lines)) {
            this.dragLine.setStyle();
        }
    }

    stopDrag(r, c) {
        if (!this.dragLine)
            return;
        
        if (this.dragLine.updateLine(this.dragStart[0],
                                     this.dragStart[1],
                                     r,
                                     c,
                                     this.lines)) {
            this.dragLine.finishUpdate(this.dragStart[0],
                                       this.dragStart[1],
                                       r,
                                       c,
                                       this.lines);
        }

        this.dragLine.setStyle();
        this.dragLine = null;
    }

    rowColumnFromMouseEventWrapper(fun) {
        var grid = this;
        return function(event) {
            event.preventDefault();
            var elem = $(document.elementFromPoint(event.pageX,
                                                   event.pageY));
            var cell = elem.data('cell');
            if (!cell) {
                return false;
            }

            return fun(cell.r, cell.c);
        };
    }

    makeStartDrag() {
        var grid = this;
        return this.rowColumnFromMouseEventWrapper(function(r, c) {
            grid.startDrag(r, c)
            return false;
        });
    }

    makePreviewDrag() {
        var grid = this;
        return this.rowColumnFromMouseEventWrapper(function(r, c) {
            grid.previewDrag(r, c)
        });
    }

    makeStopDrag() {
        var grid = this;
        return this.rowColumnFromMouseEventWrapper(function(r, c) {
            grid.stopDrag(r, c)
        });
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

    rowColumnFromTouchEventWrapper(fun) {
        var grid = this;
        return function(event) {
            event.preventDefault();
            var touch = event.changedTouches[0];
            var elem = $(document.elementFromPoint(touch.pageX,
                                                   touch.pageY));
            var cell = elem.data('cell');
            if (!cell) {
                return false;
            }

            return fun(cell.r, cell.c);
        };
    }

    makeStartDragTouch() {
        var grid = this;
        return this.rowColumnFromTouchEventWrapper(function(r, c) {
            grid.startDrag(r, c)
        });
    }

    makePreviewDragTouch() {
        var grid = this;
        return this.rowColumnFromTouchEventWrapper(function(r, c) {
            grid.previewDrag(r, c)
        });
    }

    makeStopDragTouch() {
        var grid = this;
        return this.rowColumnFromTouchEventWrapper(function(r, c) {
            grid.stopDrag(r, c)
        });
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

    reset(board) {
        var grid = this;
        board.fadeOut(250);
        board.queue(function() {
            grid.eachLine(function(line) {
                line.reset();
            });
            grid.updateStyles();
            $(this).dequeue();
        });
        board.fadeIn(250);
    }
}

class Game {
    constructor() {
        this.grid = new Grid();
        this.fp = $("#fp-container");
        this.main = $("#main-container");
        this.helpContainer = $("#help-container");
        this.aboutContainer = $("#about-container");
        this.board = $("#board");
        this.setupEvents();
    }

    setupEvents() {
        var game = this;

        // Frontpage
        function howto() {
            game.leaveFrontPage(function() {
                game.help();
            });
            return false;
        }; 
        $("#howto").click(howto);

        function about() {
            game.leaveFrontPage(function() {
                game.about();
            });
            return false;
        };        
        $("#about").click(about);

        function play() {
            game.leaveFrontPage(function() {
                game.startGame();
            });
            return false;
        }
        $("#easy").click(play);
        $("#medium").click(play);
        $("#hard").click(play);

        // Help
        function back() {
            game.back();
            return false;
        }
        $("#back-help").click(back);

        // About
        $("#back-about").click(back);

        // Game
        var grid = this.grid;
        var board = this.board;

        board.mousedown(grid.makeStartDrag());
        board.mouseup(grid.makeStopDrag());
        board.mousemove(grid.makePreviewDrag());
        board.mouseleave(grid.makeCancelDrag());

        board.on('touchstart', grid.makeStartDragTouch());
        board.on('touchend', grid.makeStopDragTouch());
        board.on('touchmove', grid.makePreviewDragTouch());
        board.on('touchleave', grid.makeCancelDrag());
        board.on('touchcancel', grid.makeCancelDrag());

        $("#reset").click(function() { grid.reset(board) });
        $("#done").click(function() { return false; });
        $("#back").click(back);
    }

    startGame() {
        var game = this;

        this.main.fadeIn(250);
        this.current = this.main;
        document.location.hash = "game";

        var grid = this.grid;
        var board = this.board;
        
        var size = grid.load([
            "   .2    ",
            "   .  5. ",
            "    2.   ",
            "    .    ",
            "    2.4  ",
            "   ..4  4",
            "  .2  . 3",
            "   . 3.  ",
            "   2 4   ",
            "  5. .2  ",
        ]);

        board.empty();
        grid.eachLine(function (line) {
            board.append(line.elem);
        });
        grid.eachCell(function (cell) {
            board.append(cell.elem);
            cell.elem.text(cell.value);
        });

        var width = cellSize * size.width + 6;
        var height = cellSize * size.height + 6;
        board.css("width", width);
        board.css("height", height);

        grid.updateStyles();

        function resize() {
            var scale =
                Math.min(Math.min(($(window).innerHeight() - 32 - 128) / height,
                                  ($(window).innerWidth() - 32) / width),
                         2);
            var bc = $("#board-container");
            bc.css("width", board.width() * scale);
            bc.css("height", board.height() * scale);
            bc.css("left", (game.main.width() - bc.width()) / 2);
            game.main.css("left", ($(window).innerWidth() - game.main.width()) / 2);

            board.css("transform", "scale(" + scale + ")");
            board.css("transform-origin", "0 0");
        }
        resize();
        $(window).resize(resize);
    }

    help() {
        this.helpContainer.fadeIn(250);
        this.current = this.helpContainer;
        document.location.hash = "help";
    }

    about() {
        $(this.aboutContainer).show();
        this.current = this.aboutContainer;
        document.location.hash = "about";
    }

    back() {
        var game = this;
        game.current.hide();
        game.frontPage();
    }

    frontPage() {
        document.location.hash = "fp";
        this.fp.fadeIn(250);
        this.current = this.fp;
    }

    leaveFrontPage(cb) {
        this.fp.hide();
        cb();
    }    
}

function preventDefault(e){
    e.preventDefault();
}

function init() {
    var game = new Game();

    var hash = document.location.hash;
    $("body").mousedown(function() { return false });

    // Fucking iOS Safari.
    $(document).on("gesturestart", preventDefault);
    document.addEventListener(
        'touchstart',
        function() {
            var touch = event.changedTouches[0];
            var elem = $(document.elementFromPoint(touch.pageX,
                                                   touch.pageY));
            if (!$(elem).hasClass("button"))
                event.preventDefault();
        },
        { passive: false });
    document.body.addEventListener('touchmove', preventDefault,
                                   { passive: false });

    if (hash == "#game") {
        game.startGame();
    } else if (hash == "#help") {
        game.help();
    } else if (hash == "#about") {
        game.about();
    } else {
        game.frontPage();
    }
}
