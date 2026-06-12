# Glourang — Oleren Native UI Library

## Philosophy

Glourang is the built-in native application library for Oleren. It targets
the same simplicity as Swing/AWT — a flat, procedural API where you build
UIs by composing widgets with function calls — but compiles to Qt6 under the
hood, giving you native rendering on every platform, hardware-accelerated
media, and real OS integration (menus, tray, file dialogs, system fonts).

No class hierarchies. No signal/slot boilerplate. No XML layouts. Just
functions that create widgets, lay them out, and attach callbacks.

```rust
@import ( ui = @std.glourang )
```

Backend: **Qt6** (`Qt6Widgets`, `Qt6Multimedia`). Importing `@std.glourang`
auto-links the required Qt modules via pkg-config. If Qt6 is not installed
the build stops with OS-specific install instructions.

---

## Status

> **Planned** — not yet implemented. This document is the API spec.

---

## Quick Start

```rust
@import ( ui = @std.glourang )

fn main() -> !void
{
    try ui.init()
    defer ui.quit()

    win := ui.window("My App", 800, 600)

    box := ui.vbox(win)
    lbl := ui.label(box, "Hello, Glourang!")
    btn := ui.button(box, "Click me")

    ui.on_click(btn) {
        ui.set_text(lbl, "Clicked!")
    }

    ui.show(win)
    ui.run()   # blocks until all windows closed
}
```

---

## Core Types

```rust
type Window = i64   # top-level window handle
type Widget = i64   # any widget (label, button, layout container, …)
type Menu   = i64   # menu bar / context menu
type Action = i64   # menu item / toolbar action
type Timer  = i64   # repeating or one-shot timer
type Image  = i64   # loaded image resource
type Media  = i64   # video or audio player handle
```

All handles are opaque integers wrapping a Qt object pointer. `0` is null /
invalid — functions that return handles return `!Widget` / `!Window` on
failure, or plain `Widget` when failure is not possible (e.g. layout
containers always succeed once the parent is valid).

---

## Application Lifecycle

```rust
ui.init() -> !void          # must be called first; initialises Qt runtime
ui.quit()                   # clean shutdown; called by defer in main
ui.run()  -> i32            # enter the event loop; returns exit code
ui.exit(code: i32)          # request loop exit with a code
ui.set_app_name(name: str)
ui.set_app_version(ver: str)
ui.set_org(name: str)       # used for settings scoping
```

---

## Windows

```rust
# Create a top-level window. Does not show it yet.
ui.window(title: str, w: i32, h: i32) -> Window

# Window control
ui.show(win: Window)
ui.hide(win: Window)
ui.close(win: Window)
ui.set_title(win: Window, title: str)
ui.set_size(win: Window, w: i32, h: i32)
ui.set_min_size(win: Window, w: i32, h: i32)
ui.set_max_size(win: Window, w: i32, h: i32)
ui.resize(win: Window, w: i32, h: i32)
ui.move(win: Window, x: i32, y: i32)
ui.center(win: Window)       # center on screen
ui.fullscreen(win: Window, on: bool)
ui.maximise(win: Window)
ui.minimise(win: Window)
ui.set_resizable(win: Window, on: bool)
ui.set_icon(win: Window, path: str)

# Query
ui.win_width(win: Window)  -> i32
ui.win_height(win: Window) -> i32

# Dialog-style windows
ui.set_modal(win: Window, on: bool)
ui.set_parent(win: Window, parent: Window)
```

---

## Layouts

Layouts are widgets that contain and arrange children. Pass a layout as the
`parent` argument to any widget or nested layout.

```rust
# Vertical / horizontal box — children stack top-to-bottom or left-to-right
ui.vbox(parent: Widget) -> Widget
ui.hbox(parent: Widget) -> Widget

# Grid — children placed at explicit (row, col) positions
ui.grid(parent: Widget) -> Widget
ui.grid_add(grid: Widget, child: Widget, row: i32, col: i32)
ui.grid_add_span(grid: Widget, child: Widget, row: i32, col: i32,
                 row_span: i32, col_span: i32)

# Stack — only one child visible at a time (tab pages, wizard steps)
ui.stack(parent: Widget) -> Widget
ui.stack_show(stack: Widget, index: i32)
ui.stack_index(stack: Widget) -> i32

# Scroll area — scrollable container for any widget
ui.scroll(parent: Widget) -> Widget

# Splitter — two panes, user-resizable divider
ui.hsplit(parent: Widget) -> Widget
ui.vsplit(parent: Widget) -> Widget

# Stretch / spacer — pushes siblings to opposite edge
ui.stretch(parent: Widget)
ui.spacer(parent: Widget, size: i32)

# Spacing and margins on any layout
ui.set_spacing(layout: Widget, px: i32)
ui.set_margin(layout: Widget, px: i32)
ui.set_margins(layout: Widget, left: i32, top: i32, right: i32, bottom: i32)
```

---

## Core Widgets

### Label

```rust
ui.label(parent: Widget, text: str) -> Widget
ui.set_text(w: Widget, text: str)
ui.get_text(w: Widget) -> str
ui.set_rich_text(w: Widget, html: str)   # basic HTML subset
ui.set_word_wrap(w: Widget, on: bool)
ui.set_align(w: Widget, align: align)

enum align => i32 {
    LEFT=0, CENTER=1, RIGHT=2,
    TOP=3, BOTTOM=4, VCENTER=5,
}
```

### Button

```rust
ui.button(parent: Widget, text: str) -> Widget
ui.icon_button(parent: Widget, icon_path: str, text: str) -> Widget
ui.toggle(parent: Widget, text: str) -> Widget    # checkable button
ui.set_enabled(w: Widget, on: bool)
ui.set_checked(w: Widget, on: bool)
ui.is_checked(w: Widget) -> bool
```

### Text Input

```rust
ui.input(parent: Widget, placeholder: str) -> Widget
ui.password(parent: Widget, placeholder: str) -> Widget
ui.set_text(w: Widget, text: str)
ui.get_text(w: Widget) -> str
ui.set_max_len(w: Widget, n: i32)
ui.set_read_only(w: Widget, on: bool)
ui.clear(w: Widget)
```

### Text Area

```rust
ui.text_area(parent: Widget, placeholder: str) -> Widget
ui.set_text(w: Widget, text: str)
ui.get_text(w: Widget) -> str
ui.append_text(w: Widget, text: str)
ui.set_read_only(w: Widget, on: bool)
ui.set_mono(w: Widget, on: bool)   # monospace font
```

### Checkbox & Radio

```rust
ui.checkbox(parent: Widget, text: str) -> Widget
ui.radio(parent: Widget, text: str) -> Widget    # group by parent vbox/hbox
ui.set_checked(w: Widget, on: bool)
ui.is_checked(w: Widget) -> bool
```

### Slider & Spinner

```rust
ui.slider(parent: Widget, min: i32, max: i32, orient: orient) -> Widget
ui.spinner(parent: Widget, min: i32, max: i32, step: i32)     -> Widget
ui.fspinner(parent: Widget, min: f64, max: f64, step: f64)    -> Widget

enum orient => i32 { HORIZ=0, VERT=1 }

ui.set_value(w: Widget, v: i32)
ui.get_value(w: Widget) -> i32
ui.set_fvalue(w: Widget, v: f64)
ui.get_fvalue(w: Widget) -> f64
```

### Combo Box (Dropdown)

```rust
ui.combo(parent: Widget) -> Widget
ui.combo_add(w: Widget, item: str)
ui.combo_add_icon(w: Widget, icon_path: str, item: str)
ui.combo_clear(w: Widget)
ui.combo_index(w: Widget) -> i32
ui.combo_text(w: Widget)  -> str
ui.combo_set(w: Widget, index: i32)
```

### List View

```rust
ui.list(parent: Widget) -> Widget
ui.list_add(w: Widget, text: str)
ui.list_add_icon(w: Widget, icon_path: str, text: str)
ui.list_remove(w: Widget, index: i32)
ui.list_clear(w: Widget)
ui.list_index(w: Widget) -> i32
ui.list_text(w: Widget, index: i32) -> str
ui.list_count(w: Widget) -> i32
ui.set_multi_select(w: Widget, on: bool)
ui.list_selected(w: Widget) -> []i32   # indices of selected items
```

### Table

```rust
ui.table(parent: Widget, rows: i32, cols: i32) -> Widget
ui.table_set(w: Widget, row: i32, col: i32, text: str)
ui.table_get(w: Widget, row: i32, col: i32) -> str
ui.table_set_header(w: Widget, col: i32, text: str)
ui.table_add_row(w: Widget)
ui.table_remove_row(w: Widget, row: i32)
ui.table_row_count(w: Widget) -> i32
ui.table_selected_row(w: Widget) -> i32
```

### Progress Bar

```rust
ui.progress(parent: Widget, min: i32, max: i32) -> Widget
ui.set_value(w: Widget, v: i32)
ui.set_indeterminate(w: Widget, on: bool)   # animated spinner mode
```

### Separator

```rust
ui.separator(parent: Widget, orient: orient) -> Widget
```

### Tabs

```rust
ui.tabs(parent: Widget) -> Widget
ui.tab_add(tabs: Widget, label: str) -> Widget   # returns the tab's container
ui.tab_add_icon(tabs: Widget, icon_path: str, label: str) -> Widget
ui.tab_index(tabs: Widget) -> i32
ui.tab_set(tabs: Widget, index: i32)
ui.tab_set_label(tabs: Widget, index: i32, label: str)
```

### Canvas (Custom Drawing)

```rust
ui.canvas(parent: Widget, w: i32, h: i32) -> Widget

# Drawing — call inside an on_paint callback
ui.draw_rect(cv: Widget, x: f32, y: f32, w: f32, h: f32, c: Color)
ui.draw_rect_outline(cv: Widget, x: f32, y: f32, w: f32, h: f32,
                     thick: f32, c: Color)
ui.draw_circle(cv: Widget, cx: f32, cy: f32, r: f32, c: Color)
ui.draw_line(cv: Widget, x1: f32, y1: f32, x2: f32, y2: f32,
             thick: f32, c: Color)
ui.draw_text(cv: Widget, text: str, x: f32, y: f32, size: f32, c: Color)
ui.draw_image(cv: Widget, img: Image, x: f32, y: f32, w: f32, h: f32)
ui.canvas_update(cv: Widget)   # trigger repaint
```

---

## Media

Glourang handles images, video, and audio natively via Qt Multimedia and
Qt's image pipeline. Supported formats depend on the platform but always
include the common ones.

### Images

```rust
# Supported: jpg, jpeg, png, gif, webp, bmp, svg, tiff, ico
ui.load_image(path: str) -> !Image
ui.unload_image(img: Image)

# Display an image in a label-like widget; respects layout sizing
ui.image_view(parent: Widget, img: Image) -> Widget
ui.image_view_path(parent: Widget, path: str) -> !Widget

ui.set_image(w: Widget, img: Image)
ui.set_image_path(w: Widget, path: str) -> !void
ui.set_image_scale(w: Widget, mode: scale_mode)
ui.set_image_url(w: Widget, url: str) -> !void   # http/https

enum scale_mode => i32 {
    NONE    = 0,   # original size, clip if too large
    FIT     = 1,   # fit inside bounds, keep aspect ratio
    FILL    = 2,   # fill bounds, keep aspect ratio, may clip
    STRETCH = 3,   # stretch to fill exactly
}

# Animated GIF / WebP — plays automatically when set
ui.set_animated(w: Widget, path: str) -> !void
```

### Video

```rust
# Create a video player widget (renders inline, controls optional)
ui.video(parent: Widget) -> Widget

ui.video_open(w: Widget, path: str) -> !void     # local file
ui.video_open_url(w: Widget, url: str) -> !void  # http/https/rtsp

ui.play(w: Widget)
ui.pause(w: Widget)
ui.stop(w: Widget)
ui.seek(w: Widget, secs: f64)

ui.set_volume(w: Widget, vol: f32)    # 0.0 - 1.0
ui.set_muted(w: Widget, on: bool)
ui.set_loop(w: Widget, on: bool)
ui.set_rate(w: Widget, rate: f32)     # 1.0 = normal, 2.0 = 2x, etc.
ui.set_controls(w: Widget, on: bool)  # show/hide built-in transport bar

ui.media_pos(w: Widget)      -> f64   # current position in seconds
ui.media_duration(w: Widget) -> f64   # total duration in seconds
ui.media_playing(w: Widget)  -> bool
ui.media_paused(w: Widget)   -> bool

# Supported formats: mp4, mkv, webm, avi, mov, flv, ogg, and
# any codec/container supported by the platform's Qt Multimedia backend.
```

### Audio

```rust
# Standalone audio player (no visible widget)
ui.audio(path: str) -> !Media
ui.audio_url(url: str) -> !Media

ui.play(m: Media)
ui.pause(m: Media)
ui.stop(m: Media)
ui.seek(m: Media, secs: f64)
ui.close_media(m: Media)

ui.set_volume(m: Media, vol: f32)
ui.set_muted(m: Media, on: bool)
ui.set_loop(m: Media, on: bool)
ui.set_rate(m: Media, rate: f32)

ui.media_pos(m: Media)      -> f64
ui.media_duration(m: Media) -> f64
ui.media_playing(m: Media)  -> bool
ui.media_paused(m: Media)   -> bool

# Supported: mp3, wav, ogg, flac, aac, opus, m4a, and platform codecs.
```

---

## Events

Event callbacks receive no arguments by default; use closures over local
variables to pass context.

```rust
# Pointer / mouse
ui.on_click(w: Widget, fn() -> void)
ui.on_double_click(w: Widget, fn() -> void)
ui.on_right_click(w: Widget, fn() -> void)
ui.on_hover(w: Widget, fn() -> void)
ui.on_leave(w: Widget, fn() -> void)
ui.on_mouse_move(w: Widget, fn(x: i32, y: i32) -> void)

# Keyboard
ui.on_key_press(w: Widget, fn(key: i32, mods: i32) -> void)
ui.on_key_release(w: Widget, fn(key: i32, mods: i32) -> void)

# Value changes
ui.on_change(w: Widget, fn() -> void)        # text changed, slider moved, etc.
ui.on_change_str(w: Widget, fn(s: str) -> void)
ui.on_change_int(w: Widget, fn(v: i32) -> void)
ui.on_change_float(w: Widget, fn(v: f64) -> void)
ui.on_submit(w: Widget, fn() -> void)        # Enter pressed in input/text area

# Selection
ui.on_select(w: Widget, fn(index: i32) -> void)    # list, combo, tabs

# Window
ui.on_close(win: Window, fn() -> bool)    # return true to allow close
ui.on_resize(win: Window, fn(w: i32, h: i32) -> void)
ui.on_focus(w: Widget, fn() -> void)
ui.on_blur(w: Widget, fn() -> void)

# Canvas
ui.on_paint(cv: Widget, fn() -> void)     # called each repaint cycle

# Media
ui.on_media_end(w: Widget, fn() -> void)
ui.on_media_error(w: Widget, fn(msg: str) -> void)
ui.on_media_progress(w: Widget, fn(pos: f64, dur: f64) -> void)
```

---

## Menus & Toolbar

```rust
# Menu bar attached to a window
menu_bar  := ui.menu_bar(win)
file_menu := ui.menu(menu_bar, "File")
edit_menu := ui.menu(menu_bar, "Edit")

# Actions (menu items)
open_act := ui.action(file_menu, "Open…", "Ctrl+O")
save_act := ui.action(file_menu, "Save",  "Ctrl+S")
ui.action_sep(file_menu)   # separator line
quit_act := ui.action(file_menu, "Quit",  "Ctrl+Q")

# Context menu (shown on right-click)
ctx := ui.context_menu(widget)
ui.action(ctx, "Copy",  "Ctrl+C")
ui.action(ctx, "Paste", "Ctrl+V")

# Toolbar
bar    := ui.toolbar(win)
tb_new := ui.tool_action(bar, "assets/new.png",  "New")
tb_opn := ui.tool_action(bar, "assets/open.png", "Open")
ui.tool_sep(bar)

# Attach callbacks to actions
ui.on_click(open_act) { ... }
ui.on_click(quit_act) { ui.exit(0) }
ui.on_click(tb_new)   { ... }
```

---

## Dialogs

```rust
# Message boxes
ui.msg(win: Window, title: str, text: str)
ui.warn(win: Window, title: str, text: str)
ui.error(win: Window, title: str, text: str)

# Confirm dialog — returns true if user clicked OK/Yes
ui.confirm(win: Window, title: str, text: str) -> bool

# Single-line text input dialog
ui.input_dialog(win: Window, title: str, prompt: str) -> str   # "" if cancelled

# File picker dialogs
ui.file_open(win: Window, title: str, filter: str) -> str    # "" if cancelled
ui.file_save(win: Window, title: str, filter: str) -> str    # "" if cancelled
ui.dir_open(win: Window, title: str) -> str

# filter format: "Images (*.jpg *.png *.webp);;All Files (*)"

# Colour picker
ui.color_dialog(win: Window, initial: Color) -> Color
```

---

## Styling

Glourang uses a simple CSS-subset applied per-widget. Fonts and colours
can also be set with typed helpers.

```rust
# CSS-subset style sheet (Qt QSS syntax)
ui.set_style(w: Widget, css: str)

# Colour
ui.set_fg(w: Widget, c: Color)      # foreground / text colour
ui.set_bg(w: Widget, c: Color)      # background colour

# Font
ui.set_font(w: Widget, family: str, size: i32)
ui.set_font_size(w: Widget, size: i32)
ui.set_bold(w: Widget, on: bool)
ui.set_italic(w: Widget, on: bool)

# Size hints
ui.set_fixed_size(w: Widget, w: i32, h: i32)
ui.set_fixed_width(w: Widget, px: i32)
ui.set_fixed_height(w: Widget, px: i32)
ui.set_expand(w: Widget, horiz: bool, vert: bool)
ui.set_visible(w: Widget, on: bool)

# Tooltip
ui.set_tooltip(w: Widget, text: str)
```

---

## Timer

```rust
# One-shot or repeating callback
ui.timer(interval_ms: i32, fn() -> void) -> Timer     # repeating
ui.timer_once(delay_ms: i32, fn() -> void) -> Timer   # fires once
ui.timer_stop(t: Timer)
ui.timer_start(t: Timer)
```

---

## System Tray

```rust
tray := ui.tray_icon("assets/icon.png")
ui.tray_tooltip(tray, "My App")

ctx  := ui.tray_menu(tray)
ui.action(ctx, "Show", "")
ui.action(ctx, "Quit", "")

ui.tray_show(tray)
ui.tray_hide(tray)
ui.on_click(tray) { ui.show(win) }
```

---

## System Dependency

| Library | pkg-config | Arch | apt | brew |
|---|---|---|---|---|
| Qt6 Widgets | `Qt6Widgets` | `qt6-base` | `qt6-base-dev` | `qt` |
| Qt6 Multimedia | `Qt6Multimedia` | `qt6-multimedia` | `qt6-multimedia-dev` | `qt` |
| Qt6 SVG | `Qt6Svg` | `qt6-svg` | `libqt6svg6-dev` | `qt` |

---

## Full Example — Media Browser

```rust
@import ( ui = @std.glourang )

fn main() -> !void
{
    try ui.init()
    defer ui.quit()

    win := ui.window("Media Browser", 1200, 800)
    ui.set_icon(win, "assets/icon.png")

    # Menu bar
    bar      := ui.menu_bar(win)
    file_m   := ui.menu(bar, "File")
    open_act := ui.action(file_m, "Open File…", "Ctrl+O")
    ui.action_sep(file_m)
    quit_act := ui.action(file_m, "Quit", "Ctrl+Q")

    # Root layout
    root  := ui.hbox(win)
    left  := ui.vbox(root)
    right := ui.vbox(root)
    ui.set_fixed_width(left, 280)

    # Left panel — file list
    search := ui.input(left, "Search…")
    flist  := ui.list(left)
    ui.set_expand(flist, false, true)

    # Right panel — viewer tabs
    viewer_tabs := ui.tabs(right)
    img_tab     := ui.tab_add(viewer_tabs, "Image")
    vid_tab     := ui.tab_add(viewer_tabs, "Video")
    aud_tab     := ui.tab_add(viewer_tabs, "Audio")

    # Image tab
    img_view := ui.image_view_path(img_tab, "assets/placeholder.png") catch |_| {
        ui.image_view(img_tab, @i64(0))
    }
    ui.set_image_scale(img_view, ui.scale_mode.FIT)

    # Video tab
    vid_view := ui.video(vid_tab)
    ui.set_controls(vid_view, true)
    ui.set_expand(vid_view, true, true)

    # Audio tab
    aud_box  := ui.vbox(aud_tab)
    aud_lbl  := ui.label(aud_box, "No file loaded")
    ui.set_align(aud_lbl, ui.align.CENTER)
    aud_prog := ui.progress(aud_box, 0, 1000)
    ctl_row  := ui.hbox(aud_box)
    play_btn := ui.button(ctl_row, "Play")
    pause_btn := ui.button(ctl_row, "Pause")
    stop_btn  := ui.button(ctl_row, "Stop")
    vol_sld   := ui.slider(aud_box, 0, 100, ui.orient.HORIZ)
    ui.set_value(vol_sld, 80)

    aud_media := ui.audio("") catch |_| { @i64(0) }

    # Events
    ui.on_click(quit_act) { ui.exit(0) }

    ui.on_click(open_act) {
        path :: ui.file_open(win, "Open Media",
            "Images (*.jpg *.jpeg *.png *.gif *.webp *.bmp *.svg);;" +
            "Video (*.mp4 *.mkv *.webm *.mov *.avi);;" +
            "Audio (*.mp3 *.wav *.ogg *.flac *.aac *.opus);;" +
            "All Files (*)")
        if path != "" { ui.list_add(flist, path) }
    }

    ui.on_select(flist) |idx| {
        path :: ui.list_text(flist, idx)
        # naive ext check — real impl would use std.fs or mime detection
        if path.len > 4 {
            ui.set_image_path(img_view, path) catch |_| {}
            ui.video_open(vid_view, path) catch |_| {}
        }
    }

    ui.on_change_int(vol_sld) |v| {
        ui.set_volume(aud_media, @f32(v) / 100.0)
    }

    ui.on_click(play_btn)  { ui.play(aud_media) }
    ui.on_click(pause_btn) { ui.pause(aud_media) }
    ui.on_click(stop_btn)  { ui.stop(aud_media) }

    ui.on_media_progress(vid_view) |pos, dur| {
        if dur > 0.0 {
            ui.set_value(aud_prog, @i32(pos / dur * 1000.0))
        }
    }

    ui.on_close(win) { ui.exit(0); ret true }

    ui.show(win)
    ui.run()
}
```
