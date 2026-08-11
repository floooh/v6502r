<div align="center">

![MacOS status](https://img.shields.io/github/actions/workflow/status/goossens/ImGuiColorTextEdit/macos.yml?branch=master&label=MacOS&style=for-the-badge)
![Linux status](https://img.shields.io/github/actions/workflow/status/goossens/ImGuiColorTextEdit/linux.yml?branch=master&label=Linux&style=for-the-badge)
![Windows status](https://img.shields.io/github/actions/workflow/status/goossens/ImGuiColorTextEdit/windows.yml?branch=master&label=Windows&style=for-the-badge)
<br/>
![Repo size](https://img.shields.io/github/repo-size/goossens/ImGuiColorTextEdit?style=for-the-badge)
![Repo activity](https://img.shields.io/github/commit-activity/m/goossens/ImGuiColorTextEdit?label=Commits&style=for-the-badge)
<br/>
[![License](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)
![Maintained](https://img.shields.io/maintenance/yes/2026?style=for-the-badge)

# Colorizing Text Editor and Text Diff for Dear ImGui

</div>

ImGuiColorTextEdit is a syntax highlighting text editor for
[Dear ImGui](https://github.com/ocornut/imgui) which was originally developed by
[Balázs Jákó](https://github.com/BalazsJako/ImGuiColorTextEdit). Unfortunately, he no
longer has time to work on the project. In fact, the last update to his repository was
in June 2019. As a result, over 200 forks exist by people who like his work but want
to fix bugs and/or add new features. A fork by
[Santiago](https://github.com/santaclose/ImGuiColorTextEdit), also known as santaclose,
was the most actively maintained version (over 220 commits ahead of the original version)
when I was looking for a syntax highlighting text editor for Dear ImGui in late 2023.
This repository was originally a fork of Santiago's version and I tried to enhance it.

In late December 2024, I decided that it was better for me to rewrite the code from
the ground up while preserving the majority of the public APIs so it would be relatively
simple for me (and others) to reuse my rewrite. One of my fundamental arguments
for the rewrite was that the code had become hard to read and maintain (at least for me).
I decided to create a more object-oriented internal architecture (see below) that made
it clear who was responsible for what by using a layered approach. While rewriting
the code, a number of new features were also added.

In April 2026, another large refactoring effort was undertaken to improve the
architecture of the widget so that new features could be implemented. The biggest change
was to make a distinction between document positions and visual positions (see below)
which makes the editor use more of a Model-View-Controller (MVC) design pattern.
This change enables implementation of Word Wrap, Line Folding and Minimaps but it breaks some
API calls. We believe the public API is now better and more logical but it takes a little
effort to port from the older versions to this new one. If you only want the old interface
and forgo new features, the legacy release contains a version that is most compatible with
older version of this editor. This legacy release will however not be maintained so you
are warned.

The rewrites and now the maintenance of this code are part of a larger project
([ObjectTalk](https://github.com/goossens/ObjectTalk)) and this repository is simply
a snapshot of the relevant editor code to provide reuse. As part of this code is
automatically generated, references to the master project are provided where required.
You can find all text editor source code components in the
[ObjectTalk repository](https://github.com/goossens/ObjectTalk/tree/master/3rdparty/imguieditor).

![Screenshot](docs/textEditor.png)

Note: In the screenshot above, the menu and status bars are not part of the
text editor widget. They are part of the complete [example application](example/)
also included in this repository. By not putting those things in the editor,
integrators have maximum flexibility to wrap the editor in their own context
in the way they see fit. Public API calls to externally implement these
features are however included.

## Features

- Works on MacOS, Linux and Windows.
- Has look and feel similar to Visual Studio Code.
- Works with latest Dear ImGui version (currently v1.92.8 && v1.92.9) and does not use deprecated functions.
- Supports dynamic font sizes (courtesy of Dear ImGui v1.92+). Implemented in the [example application](example/).
- Supports UTF-8 encoding with 16/32 bit codepoints (based on Dear ImGui configuration, see below).
- Is C++17 based (not unreasonable in 2026 I think) although Dear ImGui still uses C++11.
- Has no runtime dependencies other than Dear ImGui and the C++17 Standard Template Library (STL).
- Has full undo/redo capabilities and supports native clipboard with UTF-8 encoded strings.
- Has extendable syntax highlighting for multiple languages and API for custom languages.
- Has customizable color palettes (a dark and light version that work with Dear ImGui defaults are included).
- Has option to automatically insert spaces on tabs.
- Has find/replace user interface and API with full undo/redo.
- Find has options for whole word and/or case-sensitive searches.
- Has support for configurable word wrap (see [description](docs/wordwrap.md), default is off).
- Supports optional line folding like Visual Studio Code (default is off).
- Folding works on brackets (for all languages) and on indentation for languages like Python.
- Provides optional autocomplete framework (see [more information here](docs/autocomplete.md), default is off).
- Has marker API to specify lines and/or line numbers to highlight and optional show tooltips (see [example](docs/markers.md)).
- Has squiggle API to underline parts of the text like Visual Studio Code to highlight and optional show tooltips.
- Has optional full minimap similar to Visual Studio Code.
- Has optional scrollbar minimap to just render current cursor, selections and marker locations.
- Provides middle-mouse pan and scroll functions like CAD programs and browsers.
- Has API to attach user data to select lines or all lines (see [example](docs/userData.md)).
- Has API to decorate each line (useful for debuggers and IDEs) (see [example](docs/lineDecorator.md)).
- Provides optional and customizable right click (control + click on MacOS) context menus for line numbers or text lines (see [example](docs/contextMenus.md))
- Provides a glyph hover-over help system (see [example](docs/hover.md)).
- Provides auto completion for paired glyphs (\[, \{, \(, \", \') (can be turned on and off).
- If auto complete is turned on, accidentally typed closing glyphs are ignored.
- If auto complete is turned on, selections can be surrounded by paired glyphs.
- Supports bracket matching and coloring similar to Visual Studio Code (can be turned on and off).
- Supports multiple cursors and multiple selections.
- Provides auto indent with simplified implementation (can be turned on and off).
- Has API to filter selections (with full undo support).
- Uppercase/lowercase filter is example of selection filtering.
- Has API to filter each line in editor (with full undo support).
- Tabs to Space (and visa versa) are examples of line filtering.
- Has API to strip trailing whitespaces.
- Provides whitespace indicators for tabs and spaces (can be turned on and off individually or collectively).
- Supports blinking caret(s)/cursor(s) (can be turned on/off using Dear ImGui's global io.ConfigInputTextCursorBlink flag).
- Supports caret(s)/cursor(s) hiding through API (default is that they are visible even in readonly mode).
- Supports custom caret rendering through a callback (see [example](docs/customCaret.md)).
- No longer uses regular expressions for colorizing text (see below).
- Provides an example Language Server Protocol (LSP) bridge (see [example](docs/lsp.md)).
- Provides an optional companion widget to show differences between versions of text (see below).

## Integration

As explained above, the editor is developed and maintained as part of a larger project.
In that bigger project, the editor is spread out over multiple source files and some
of those are automatically generated using tools or derived from datasets (like the
unicode database). Every time the editor is updated in the bigger project, relevant
files are concatenated into a simple 2 (or 5 if you also want to use TextDiff)
distribution which is included here.

This repository therefore provides a simple mechanism to reuse the editor in any
Dear ImGui context by doing the following:

- Include the TextEditor.cpp and TextEditor.h files in your project.
- Instantiate a TextEditor object for each editor widget you need.
- Use the public API to set editor options or interact with the editor's contents.
- Call the TextEditor's Render member function every frame in your Dear ImGui loop.
- If you plan to use non-ASCII characters in your text, see the Unicode section below.
- Configure Dear ImGui's clipboard functions since that is what this editor uses.

If you also want to use the TextDiff widget, you must:

- Also include TextDiff.cpp, TextDiff.h and dtl.h in your project.
- Instantiate a TextDiff object for each diff widget you need.
- Use the public API to set diff options.
- Call the TextDiff's Render member function every frame in your Dear ImGui loop.
- If you properly configured Dear ImGui for the TextEditor (see above), you are good to go.

For a complete example of both widgets, please see the [example folder](example/).

**Note**: Like ImGui, it is recommended that you compile and link TextEditor and TextDiff
as a static library or directly as part of your sources (as suggested above).
However, if you must and are compiling TextEditor and/or TextDiff as separate DLLs, make sure
you set the current ImGui context with TextEditor::SetImGuiContext(ImGuiContext* ctx).
This ensures that global ImGui variables are correctly shared across the DLL boundary.
See GImGui documentation in imgui.cpp for more details.

## Default Keyboard and Mouse Mappings

- In the mappings listed below, the following modifier keys are used:
	- Ctrl: this refers to the Command key on MacOS and the Control key on Linux and Windows.
	- Alt: this refers to the Option key on MacOS and the Alt key on Linux and Windows.
	- Shift: as you would expect on all platforms.
	- If a keyboard has a left and right version of these modifiers, the meaning is not different.

- Cursor Movements:
	- Single left mouse click moves the current cursor and cancels additional cursors.
	- Arrow keys move cursor left, right, up and down.
	- PageUp and PageDown do what you expect.
	- Ctrl-UpArrow and Ctrl-DownArrow move to the start/end of the document.
	- Home and End keys move to the start/end of the line.
	- Holding down the Alt key with the left or right arrow moves a whole word on MacOS.
	- Holding down the Ctrl key with the left or right arrow moves a whole word on Linux and Windows.

- Panning and Scrolling:
	- The text scrolls automatically when you move the cursor through keyboard actions.
	- Mouse actions that extend the selections also apply auto scrolling.
	- The text in the editor can still be scrolled using those little bars that were invented in the 1970's.
	- Devices with scroll wheels or those that simulated vertical and horizontal scroll wheels (like a touch pad, a mouse with a builtin touch pad or a pen) can also scroll the text. This is actually implemented in Dear ImGui (and used by the editor) and must be supported by your backend.
	- The middle mouse button on a three-button mouse (or whatever is reported by your OS as a middle mouse button event) enters pan or scroll mode mode depending on the configuration. Pan mode is the default and you can switch this to Scroll mode by calling SetMiddleMouseScrollMode(). Calling SetMiddleMousePanMode() switches it back. The [example application](example/) uses a menu option to toggle modes.
	- In pan mode, the text is grabbed and dragged as the cursor moves and as long as the middle mouse button is down.
	- When you mouse approaches the edges of the editor window, it enters continuous panning mode and the further you move away form the window, the faster it pans.
	- Panning as described above is typical in CAD or 3D type applications.
	- In scroll mode, you can release the middle mouse button and scroll the text just like you can in some browsers.
	- Scroll mode is ended by any clicking any mouse button.
	- Panning and scrolling operate in opposite directions as they are different paradigms.
	- An optional indicator (default is on) is shown in the center of the editor window when entering pan/scroll mode. If anybody finds this annoying, it can be turned off through an API by calling SetShowPanScrollIndicatorEnabled(false).

- Cursors and Selections:
	- Alt with single left mouse click creates a new cursor on MacOS.
	- Ctrl with single left mouse click creates a new cursor on Linux and Windows.
	- Ctrl-A selects all text.
	- Ctrl-D creates a new cursor for the next instance of the current cursor's text.
	- Shift-Ctrl-D creates a new cursor for each instance of the current cursor's text.
	- Double left mouse clicks on a curly bracket select the content of the relevant block and replaces all previous cursors.
	- Shift + Double left mouse clicks on a curly bracket select the content of the relevant block including the brackets and replaces all previous cursors.
	- Double left mouse clicks not on a bracket or parenthesis, select a word. Adding Shift extends current selection.
	- Triple left mouse clicks select a line. Adding Shift extends current selection.
	- Dragging mouse with left mouse button selects text. Shift extends current selection.
	- Super-Shift-RightArrow (on MacOS) and Alt-Shift-RightArrow (on Linux and Windows) grows all selections. First the current word, than just the content of a bracketed block, than including the  brackets. Continuously hitting the key combination keeps growing the selections.
	- Super-Shift-LeftArrow (on MacOS) and Alt-Shift-LeftArrow (on Linux and Windows) shrinks all selections. First the bracketed block, that just the content of the block. Continuously hitting the key combination keeps shrinking the selections.
	- Left mouse clicking or dragging over line numbers select line(s).
	- Left mouse clicking on minimap sets the cursor the the selected position and cancels additional cursors.
	- When multiple cursors are active, hitting the ESC key cancels them.

- Clipboard Operations:
	- Ctrl-X or Shift-Delete cuts selected text or current line if no selection.
	- Ctrl-C or Ctrl-Insert copies selected text or current line if no selection.
	- Ctrl-V or Shift-Insert pastes clipboard content.
	- Ctrl-Z undoes the last action.
	- Shift-Ctrl-Z or Ctrl-Y redos the last undone action.

- Insert Text:
	- Ctrl-Enter adds line below.
	- Ctrl-Shift-Enter adds line above.

	- Delete Text:
	- Backspace key deletes character to the left of cursor or deletes selection(s).
	- Delete key deletes character to the right of cursor or deletes selection(s).
	- Ctrl-Shift-K deletes all selected lines.

- Text Manipulation:
	- Ctrl-] indents current line or current lines.
	- Ctrl-[ de-indents current line or current lines.
	- Tab with selection indents current line or current lines.
	- Shift-Tab de-indents current line or current lines.
	- Alt-UpArrow moves current or all selected lines up.
	- Alt-DownArrow moves current or all selected lines down.
	- Ctrl-/ or Ctrl-L toggle line comments for current or selected lines if language with single line comments is specified.

- Find & Replace:
	- Ctrl-F opens the find and replace window.
	- If text is selected when Ctrl-F is hit, the selected text is used as the search term if the start and end are on the same line. Else if the cursor is in a word when Ctrl-F is hit, that word is used as the search term.
	- In all other cases, the previous search term is used.
	- When the search and replace window is visible, hitting the ESC button closes it.
	- Shift-Ctrl-F finds all instances and makes them separate cursors.
	- Ctrl-G finds the next instance of the search term.

- AutoComplete:
	- Once configured, Ctrl-space (or a custom key combination) triggers a manual autocomplete (even on MacOS as Cmd-space globally triggers Spotlight searching on that platform).
	- Typing also triggers autocomplete (based on context).
	- You can configure which triggers you want (the default is for both manual and automatic (by just typing) to be active).
	- AutoComplete does not work when multiple cursors are active.

- Other:
	- Insert key toggles between insert (default) and overwrite modes.

## Unicode and UTF-8

This rewrite of TextEditor uses UTF-8 encoded strings for the public API
and to interact with the clipboard like the original TextEditor versions.
The original and most of its forks use UTF-8 on the private side as well
which is effective from a storage perspective but it makes all the
processing a lot harder as you constantly have to parse UTF-8 sequences.

This rewrite internally uses unicode codepoints that are either 16 or 32 bits
depending on how Dear ImGui is configured. If IMGUI_USE_WCHAR32 is defined,
Dear ImGui as well as this editor use 32 bits to store a codepoint. If it is not
defined (which is the default), 16 bits are used for each codepoint which
basically limits unicode support to the Basic Multilingual Plane (see
[this article on Wikipedia](https://en.wikipedia.org/wiki/Plane_(Unicode))).

Regardless of whether 16 or 32 bit codepoints are used, it is your responsibility
to ensure Dear ImGui is configured with the correct font and font size. As of
Dear ImGui v1.92, you can use dynamically sized fonts and use any font glyph
without setting up glyph ranges or a font atlas yourself. All this functionality
is in the Dear ImGui core and the TextEditor uses it. Please see the
[example application](example/) on how to use PushFont/PopFont around the
TextEditor Render function. The demo app also shows how to increase/decrease
the font size on the fly.

## Architecture

While this editor relies on [Omar Cornut's Dear ImGui](https://github.com/ocornut/imgui),
it does not follow the "pure" one widget - one function approach. Since the editor
has to maintain a relatively complex and large internal state, it did not seem
to be practical to try and enforce full immediate mode. It therefore stores
its internal state in an object instance which is reused across frames. This
object is an instance of the TextEditor class which not only stores the
internal state, it also provides the public API. The "Render" member functions
however works like a normal Dear ImGui widget. It creates a child window to
render the editor so you can call Dear ImGui window functions like SetNextWindowSize
or set the font before calling Render.

For a more detailed description of the architecture, please see the
[documentation](docs/architecture.md).

## TextDiff

TextDiff is a separate widget that allows you to show the differences between
two versions of some text while preserving color highlighting. The code for this
widget is in separate files (see integration above) so it is optional. TextDiff
is not a standalone widget as it "borrows" code and algorithms from TextEditor.
If you use TextDiff, you must also include TextEditor.h and TextEditor.cpp in
your project. TextDiff is readonly and has two modes:

- **Integrated view** where differences are depicted vertically using the standard "diff" look and feel. This is the default view.

- **Side-by-side view** where the original and altered versions are shown/compared side-by-side.

In both modes, the provided text can be colored based on a specified language and
color palette like in the TextEditor. The background colors for the difference
highlighting are not part of the palette but can still be changed through a the public API.

If scrolling is required, you can use the scrollbars, scroll wheel(s) (physical or logical
as reported by Dear ImGui) or the following keyboard shortcuts when the widget is focussed:
Arrow keys, home, end/ page up and page down.

Below are two screenshots of its use in both modes. Have a look at the code in the
[example application](example/) to see how easy it is to use this TextDiff widget.

![Screenshot 2](docs/textDiffCombined.png)
![Screenshot 3](docs/textDiffSideBySide.png)

## Extras

This repository contains some optional companion classes for a simple autocomplete
feature, a bridge to language servers and on-screen notifications. Details on simple
autocomplete can be found [here](docs/autocomplete.md) and details on the language
server bridge are [here](docs/lsp.md). Details of the "toast" notification system
are [here](extras/Notifications.h). The source code for these components are in the
[extras folder](extras/) and the [example application](example/) shows how
to use them.

## External Examples

- [Pascal Thomet](https://github.com/pthom) has integrated this editor in his
[Dear ImGui Bundle](https://github.com/pthom/imgui_bundle) and he has a very
nice online [web-based demonstration](https://imgui-bundle.pages.dev/explorer/).
You can see the editor [here](https://imgui-bundle.pages.dev/explorer/demo_text_edit)
and source code is available
[here](https://github.com/pthom/imgui_bundle/blob/main/bindings/imgui_bundle/demos_cpp/demo_text_edit.cpp).

- [Yan Pujante](https://github.com/ypujante) has integrated this editor in his
[WebGPU Shader Toy](https://pongasoft.github.io/webgpu-shader-toy/) and you can
find the source code [here](https://codeberg.org/pongasoft/webgpu-shader-toy).
This is another fine example on how far web technology has come and how easy
it is to use this editor on a web page.

## Versioning

This repository includes releases with a numbering scheme synchronized with Dear ImGui.
This will allow people to quickly find a version of the widgets compatible with a specific Dear ImGui version.

## Issues

If you are interested in using this Text Editor, steal parts of the code, make
suggestions for improvements or contribute fixes/enhancements, be my guest as this
repository is released under the MIT license. For people that want to contribute,
[Contributing Guidelines](CONTRIBUTING) and a [Code of Conduct](CODE_OF_CONDUCT.md)
are available. If you find any problems or want to make a suggestion for improvement, please
[raise an issue on this repository](https://github.com/goossens/ImGuiColorTextEdit/issues).

## Credits

This version of ImGuiColorTextEdit was written from scratch by [Johan A. Goossens](https://github.com/goossens)
and if you end up using (parts of) this repository a shoutout or Github star would be appreciated.

Thank you to [Omar Cornut](https://github.com/ocornut/imgui) for creating Dear ImGui
in the first place. Without you, this editor would not exist.

I owe a great deal of gratitude to [Balázs Jákó](https://github.com/BalazsJako/ImGuiColorTextEdit)
who started the original version of this editor many years ago.

I also have to thank [Santiago (santaclose)](https://github.com/santaclose/ImGuiColorTextEdit)
for reviving this project, merging the unfinished developments and pull requests.
He was also responsible for adding features to the editor like multiple cursors.

Given that neither Balázs nor Santiago kept a credits list, it is highly likely
that many others have also made contributions. Theoretically I could go through
all the issues on the original and the fork but I think I have better use for my
time. If you believe you deserve credit here, please raise an issue with reference
to your work. I'll gladly add it.

For the Text Diff widget, credit goes to [Tatsuhiko Kubo (cubicdaiya)](https://github.com/cubicdaiya)
for his [Diff Template Library (DTL)](https://github.com/cubicdaiya/dtl) which is
released under the BSD License.

## License

This work is licensed under the terms of the MIT license.
For a copy, see <https://opensource.org/licenses/MIT>.

The included DTL library (which is only used in the Diff Widget) is released under the
[BSD license](https://github.com/cubicdaiya/dtl/blob/master/COPYING).

Some of the unicode algorithms are derived from the
[Unicode Character Database](https://www.unicode.org/ucd/)
which is released under the [Unicode Terms of Use](https://www.unicode.org/copyright.html).
