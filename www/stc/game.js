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
        this.transposed = false;

        this.dragLeftStartSquare = false;
    }

    transpose() {
        this.transposed = !this.transposed;
        this.setStyle();
    }

    setStyle() {
        this.setLineStyle(this.h1, this.h2);
        this.elem.removeClass("blink");
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

        var height = (1 + Math.abs(h1[0] - h2[0]));
        var width = (1 + Math.abs(h1[1] - h2[1]));
        if (this.transposed) {
            this.elem.css("top", h1[1] * cellSize + 4);
            this.elem.css("left", h1[0] * cellSize + 4);
            this.elem.css("height", cellSize * width - 8);
            this.elem.css("width", cellSize * height - 8);
        } else {
            this.elem.css("top", h1[0] * cellSize + 4);
            this.elem.css("left", h1[1] * cellSize + 4);
            this.elem.css("height", cellSize * height - 8);
            this.elem.css("width", cellSize * width - 8);
        }


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
            } else if (fromR == this.r && fromC == this.c) {
                ret = [[r, c], [fromR, fromC]];
            } else {
                console.log("should not happen");
                return null;
            }
        } else {
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
            return false;
        }
        this.h1 = handles[0];
        this.h2 = handles[1];
        return true;
    }

    reset() {
        this.h1 = [this.r, this.c];
        this.h2 = [this.r, this.c];
        this.setStyle();
    }

    covered(handle) {
        if (!handle) {
            return this.covered(this.h1.slice(0)).concat(this.covered(this.h2.slice(0)));
        }

        var ret = [];
        while (true) {
            ret.push([handle[0], handle[1]]);
            this.moveTowardCenter(handle);
            if (handle[0] == this.r && handle[1] == this.c) {
                break;
            }
        } 

        return ret;
    }
}

class Cell {
    constructor(r, c, elem) {
        this.r = r;
        this.c = c;
        this.elem = elem;
        this.transposed = false;

        elem.css("top", r * cellSize);
        elem.css("left", c * cellSize);
    }

    transpose() {
        this.transposed = !this.transposed;
        if (this.transposed) {
            this.elem.css("top", this.c * cellSize);
            this.elem.css("left", this.r * cellSize);
        } else {
            this.elem.css("top", this.r * cellSize);
            this.elem.css("left", this.c * cellSize);
        }
    }

    setStyle() {
        this.elem.removeClass("blink");
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

    transpose() {
        this.eachCell(function(cell) {
            cell.transpose();
        });
        this.eachLine(function(line) {
            line.transpose();
        });
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
        this.dragAxisSet = false;
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
        var rowAxis;
        return this.rowColumnFromTouchEventWrapper(function(r, c) {
            if (grid.dragAxisSet) {
                if (rowAxis) {
                    r = grid.dragStart[0];
                } else {
                    c = grid.dragStart[1];
                }
            }
            grid.previewDrag(r, c);
            if (grid.dragLine) {
                if (grid.dragStart[0] != r ||
                    grid.dragStart[1] != c) {
                    if (!grid.dragAxisSet) {
                        if (grid.dragStart[0] != r) {
                            rowAxis = false;
                        } else {
                            rowAxis = true;
                        }
                        grid.dragAxisSet = true;
                    }
                    
                    grid.dragLine.finishUpdate(grid.dragStart[0],
                                               grid.dragStart[1],
                                               r,
                                               c,
                                               grid.lines);
                    grid.dragStart[0] = r;
                    grid.dragStart[1] = c;
                    grid.dragLeftStartSquare = false;
                }
            }
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
        this.eachLine(function(line) {
            line.setStyle();
        });
    }

    reset(board) {
        var grid = this;
        if (grid.empty()) {
            return;
        }
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

    check() {
        var ret = true;
        var covered = {}
        var blink = [];

        this.lines.forEach(function(line) {
            line.elem.removeClass("blink");
            line.covered().forEach(function(at) {
                covered[at] = true;
            });
            if (line.length != line.targetLength) {
                blink.push(line.elem);
            }
        });

        this.eachCell(function(cell) {
            cell.elem.removeClass("blink");
            if (cell.value == '.' &&
                !covered[[cell.r, cell.c]]) {
                blink.push(cell.elem);
            }
        });
        
        setTimeout(function(){
            blink.forEach(function (elem) {
                elem.addClass("blink");
            });
        }, 1);
        
        return blink.length == 0;
    }

    clear() {
        this.grid = [];
        this.lines = [];
    }

    empty() {
        return this.grid.length == 0;
    }
}

class GameId {
    constructor(id) {
        console.log(id);
        var components = id.split('.');
        this.level = components[0];
        this.index = parseInt(components[1]);
    }

    next() {
        return new GameId(this.level + "." + (this.index + 1));
    }

    str() {
        return this.level + "." + this.index;
    }
}

class Game {
    constructor(games) {
        this.games = games;
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

        function play(level, initialId) {
            game.leaveFrontPage(function() {
                var id = initialId || game.getStartLevel(level);
                // TODO: Store solved puzzles in localstorage,
                // jump to first unsolved.
                game.startGame(new GameId(level + "." + id));
            });
            return false;
        }
        $("#tutorial").click(function() { play("tutorial", 1) });
        $("#easy").click(function() { play("easy") });
        $("#medium").click(function() { play("medium") });
        $("#hard").click(function() { play("hard") });
        $("#expert").click(function() { play("expert") });

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

        $("#reset").click(function() { grid.reset(board) });
        $("#done").click(function() {
            game.grid.updateStyles();
            if (game.grid.empty()) {
                // Done on the final level of a sequence returns
                // to the front page.
                back();
            } else if (game.grid.check()) {
                game.incrementStartLevel(game.gameId);
                game.startGame(game.gameId.next());
            }
            return false;
        });

        $("#back").click(back);
    }

    incrementStartLevel(gameId) {
        var storage = window.localStorage;
        if (storage) {
            var index = gameId.index + 1;
            var stored = localStorage.getItem(gameId.level, index);
            if (stored) {
                index = Math.max(index, parseInt(stored));
            }
            localStorage.setItem(gameId.level, index);
        }
    }

    getStartLevel(level) {
        var storage = window.localStorage;
        if (storage) {
            var max = localStorage.getItem(level);
            if (max) {
                return parseInt(max);
            }
        }
        return 1;
    }

    startGame(gameId) {
        var game = this;

        this.current = this.main;
        var grid = this.grid;
        var board = this.board;

        if (!game.games[gameId.level]) {
            console.log("unknown difficulty setting: ",
                        gameId.level);
            game.frontPage();
            return;
        }

        if (gameId.index < 0 ||
            gameId.index > game.games[gameId.level].length) {
            console.log("level id out of range: ",
                        gameId.index,
                        game.games[gameId.level].length);
            game.frontPage();
            return;
        }

        var spec = game.games[gameId.level][gameId.index - 1];

        $("#message").toggle(spec.message != null);
        $("#message").text(spec.message);
        if (spec.message) {
            $("#message").css("max-width", 500);
        }
        $("#board").toggle(spec.puzzle != null);

        game.gameId = gameId;
        document.location.hash = "game." + gameId.str();

        var width, height;
        if (spec.puzzle) {
            var size = grid.load(spec.puzzle);

            board.empty();
            grid.eachLine(function (line) {
                board.append(line.elem);
            });
            grid.eachCell(function (cell) {
                board.append(cell.elem);
                if (cell.value == '.') {
                    cell.elem.html("&middot;");
                } else {
                    cell.elem.text(cell.value);
                }
            });

            width = cellSize * size.width + 6;
            height = cellSize * size.height + 6;
            board.css("width", width);
            board.css("height", height);

            grid.updateStyles();
        } else {
            grid.clear();
            board.empty();
            board.css("width", 0);
            board.css("height", 0);
            width = height = 0;
        }

        function resize() {
            var win_height = $(window).innerHeight();
            var win_width = $(window).innerWidth();
            if (board.height() != board.width() &&
                (win_height < win_width) !=
                (board.height() < board.width())) {
                var tmp = width;
                width = height;
                height = tmp;
                game.transpose();
            }
            if (win_height < 500) {
                var controls = $("#controls");
                var scale = Math.max(win_height / 500,
                                     0.1);
                controls.css("transform", "scale(" + scale + ")");
                controls.css("transform-origin", "0 0");
            }
            var scale =
                Math.min(Math.min((win_height - 64 - $("#controls").height()) / height,
                                  (win_width - 32) / width),
                         2.5);
            var bc = $("#board-container");
            bc.css("width", (board.width() || 0) * scale);
            bc.css("height", (board.height() || 0) * scale);
            bc.css("left", (game.main.width() - bc.width()) / 2);
            game.main.css("left", (win_width - game.main.width()) / 2);

            board.css("transform", "scale(" + scale + ")");
            board.css("transform-origin", "0 0");
        }
        $(window).off("resize");
        $(window).resize(resize);

        this.main.show();
        resize();
    }

    transpose() {
        this.grid.transpose();
        var w = this.board.width();
        this.board.width(this.board.height());
        this.board.height(w);
    }

    scaleFp(id) {
        var hc = $(id)
        var height = hc.height();
        var width = hc.width();

        function resize() {
            var scale =
                Math.min(Math.min(($(window).innerHeight() - 32) / height,
                                  ($(window).innerWidth() - 32) / width),
                         1.0);
            var bc = $("#board-container");
            hc.css("transform", "scale(" + scale + ")");
            hc.css("transform-origin", "0 0");
        }
        $(window).off("resize");
        $(window).resize(resize);

        return resize;
    }

    help() {
        var resize = this.scaleFp("#help-container");
        this.helpContainer.show();
        this.current = this.helpContainer;
        document.location.hash = "help";
        resize();
    }

    about() {
        var resize = this.scaleFp("#about-container");
        $(this.aboutContainer).show();
        this.current = this.aboutContainer;
        document.location.hash = "about";
        resize();
    }

    back() {
        var game = this;
        game.current.hide();
        game.frontPage();
    }

    frontPage() {
        var resize = this.scaleFp("#fp-container");
        document.location.hash = "fp";
        this.fp.show();
        this.current = this.fp;
        resize();
    }

    leaveFrontPage(cb) {
        this.fp.hide();
        cb();
    }    
}

function preventDefault(e){
    e.preventDefault();
}

function initUI(games) {
    var game = new Game(games);

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

    if (hash.startsWith("#game.")) {
        var gameid = hash.replace("#game.", "");
        game.startGame(new GameId(gameid));
    } else if (hash == "#help") {
        game.help();
    } else if (hash == "#about") {
        game.about();
    } else {
        game.frontPage();
    }
}

function init() {
    $.ajax({
        url: 'data/puzzles.json',
        dataType: 'json',
    }).done(function(response) {
        initUI(response);
    }).fail(function(err) {
        $("#error-container.div").hide();
        $("#error-loading-game-data").show();
        $("#error-container").show();
        console.log("Couldn't load games", err);
    });
}
