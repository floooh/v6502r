//	TextEditor - A syntax highlighting text editor for Dear ImGui.
//	Copyright (c) 2024-2026 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


#pragma once


//
//	Include files
//

#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "imgui.h"


//
//	TextEditor
//

class IMGUI_API TextEditor {
public:
	// constructor
	TextEditor();

	//
	// Below is the public API
	// Public member functions start with an uppercase character to be consistent with Dear ImGui
	//

	struct DocPos {
		// represents the logical position of a glyph in a document expressed as a line number and glyph index (both zero-based)
		DocPos() = default;
		DocPos(size_t line, size_t index) : line(line), index(index) {}

		inline bool operator ==(const DocPos& rhs) const { return line == rhs.line && index == rhs.index; }
		inline bool operator !=(const DocPos& rhs) const { return line != rhs.line || index != rhs.index; }
		inline bool operator <(const DocPos& rhs) const { return line != rhs.line ? line < rhs.line : index < rhs.index; }
		inline bool operator >(const DocPos& rhs) const { return line != rhs.line ? line > rhs.line : index > rhs.index; }
		inline bool operator <=(const DocPos& rhs) const { return line != rhs.line ? line < rhs.line : index <= rhs.index; }
		inline bool operator >=(const DocPos& rhs) const { return line != rhs.line ? line > rhs.line : index >= rhs.index; }

		inline DocPos operator -(const DocPos& rhs) const { return DocPos(line - rhs.line, index - rhs.index); }
		inline DocPos operator +(const DocPos& rhs) const { return DocPos(line + rhs.line, index + rhs.index); }

		size_t line = 0;
		size_t index = 0;
	};

	struct DocSelection {
		// represents a range of glyphs from a starting position to an end position
		DocSelection() = default;
		DocSelection(DocPos start, DocPos end) : start(start), end(end) {}
		DocPos start;
		DocPos end;
	};

	struct VisPos {
		// represents the visual position of a glyph expressed as a visible row and column number (both zero-based)
		// this is not necessarily the same as the document position
		// tabs create a horizontal offset between index and column
		// word-wrapping creates a vertical offset between line and row
		VisPos() = default;
		VisPos(size_t row, size_t column) : row(row), column(column) {}

		inline bool operator ==(const VisPos& rhs) const { return row == rhs.row && column == rhs.column; }
		inline bool operator !=(const VisPos& rhs) const { return row != rhs.row || column != rhs.column; }
		inline bool operator <(const VisPos& rhs) const { return row != rhs.row ? row < rhs.row : column < rhs.column; }
		inline bool operator >(const VisPos& rhs) const { return row != rhs.row ? row > rhs.row : column > rhs.column; }
		inline bool operator <=(const VisPos& rhs) const { return row != rhs.row ? row < rhs.row : column <= rhs.column; }
		inline bool operator >=(const VisPos& rhs) const { return row != rhs.row ? row > rhs.row : column >= rhs.column; }

		inline VisPos operator -(const VisPos& rhs) const { return VisPos(row - rhs.row, column - rhs.column); }
		inline VisPos operator +(const VisPos& rhs) const { return VisPos(row + rhs.row, column + rhs.column); }

		size_t row = 0;
		size_t column = 0;
	};

	// access editor's configuration options
	inline void SetTabSize(size_t value) { config.tabSize = value; }
	inline size_t GetTabSize() const { return config.tabSize; }
	inline void SetInsertSpacesOnTabs(bool value) { config.insertSpacesOnTabs = value; }
	inline bool IsInsertSpacesOnTabs() const { return config.insertSpacesOnTabs; }
	inline void SetLineSpacing(float value) { config.lineSpacing = std::max(1.0f, std::min(2.0f, value)); }
	inline float GetLineSpacing() const { return config.lineSpacing; }
	inline void SetWordWrapEnabled(bool value) { config.wordWrap = value; }
	inline bool IsWordWrapEnabled() const { return config.wordWrap; }
	inline void SetReadOnlyEnabled(bool value) { config.readOnly = value; }
	inline bool IsReadOnlyEnabled() const { return config.readOnly; }
	inline void SetCaretsVisible(bool value) { config.caretsVisible = value; }
	inline bool IsCaretsVisible() const { return config.caretsVisible; }
	inline void SetAutoIndentEnabled(bool value) { config.autoIndent = value; }
	inline bool IsAutoIndentEnabled() const { return config.autoIndent; }
	inline void SetShowWhitespacesEnabled(bool value) { config.showSpaces = value; config.showTabs = value; }
	inline bool IsShowWhitespacesEnabled() const { return config.showSpaces && config.showTabs; }
	inline void SetShowSpacesEnabled(bool value) { config.showSpaces = value; }
	inline bool IsShowSpacesEnabled() const { return config.showSpaces; }
	inline void SetShowTabsEnabled(bool value) { config.showTabs = value; }
	inline bool IsShowTabsEnabled() const { return config.showTabs; }
	inline void SetShowLineNumbersEnabled(bool value) { config.showLineNumbers = value; }
	inline bool IsShowLineNumbersEnabled() const { return config.showLineNumbers; }
	inline void SetShowMiniMapEnabled(bool value) { config.showMiniMap = value; }
	inline bool IsShowMiniMapEnabled() const { return config.showMiniMap; }
	inline void SetMiniMapColumns(size_t value) { config.miniMapColumns = value; }
	inline size_t GetMiniMapColumns() const { return config.miniMapColumns; }
	inline void SetShowScrollbarMiniMapEnabled(bool value) { config.showScrollbarMiniMap = value; }
	inline bool IsShowScrollbarMiniMapEnabled() const { return config.showScrollbarMiniMap; }
	inline void SetShowPanScrollIndicatorEnabled(bool value) { config.showPanScrollIndicator = value; }
	inline bool IsShowPanScrollIndicatorEnabled() const { return config.showPanScrollIndicator; }
	inline void SetShowMatchingBrackets(bool value) { config.showMatchingBrackets = value; if (!value) { config.lineFolding = false; } }
	inline bool IsShowingMatchingBrackets() const { return config.showMatchingBrackets; }
	inline void SetCompletePairedGlyphs(bool value) { config.completePairedGlyphs = value; }
	inline bool IsCompletingPairedGlyphs() const { return config.completePairedGlyphs; }
	inline void SetLineFoldingEnabled(bool value) { config.lineFolding = value; if (value) { config.showMatchingBrackets = true; } }
	inline bool IsLineFoldingEnabled() const { return config.lineFolding; }
	inline void SetOverwriteEnabled(bool value) { config.overwrite = value; }
	inline bool IsOverwriteEnabled() const { return config.overwrite; }
	inline void SetMiddleMousePanMode() { config.panMode = true; }
	inline void SetMiddleMouseScrollMode() { config.panMode = false; }
	inline bool IsMiddleMousePanMode() const { return config.panMode; }
	inline void SetLineNumberLeftMargin(size_t value) { config.leftMargin = value; } // margins are expressed in glyphs
	inline size_t GetLineNumberLeftMargin() const { return config.leftMargin; }
	inline void SetDecorationLeftMargin(size_t value) { config.decorationMargin = value; }
	inline size_t GetDecorationLeftMargin() const { return config.decorationMargin; }
	inline void SetTextLeftMargin(size_t value) { config.textMargin = value; }
	inline size_t GetTextLeftMargin() const { return config.textMargin; }

	// access text (using UTF-8 encoded strings)
	// (see note below on cursor and scroll manipulation after setting new text)
	inline void SetText(const std::string_view& text) { setText(text); }
	inline std::string GetText() const { return document.getText(); }

	inline std::string GetCursorText(size_t cursor) const { return cursor < cursors.size() ? document.getSectionText(cursors[cursor].getSelectionStart(), cursors[cursor].getSelectionEnd()) : ""; }
	inline std::string GetLineText(size_t line) const { return line < document.size() ? document.getLineText(line) : ""; }
 	inline std::string GetSectionText(DocPos start, DocPos end) const { return document.getSectionText(normalizePos(start), normalizePos(end)); }
	inline std::string GetSectionText(const DocSelection& selection) const { return GetSectionText(selection.start, selection.end); }
	inline void ReplaceSectionText(DocPos start, DocPos end, const std::string_view& text) { replaceSectionText(normalizePos(start), normalizePos(end), text); }
	inline void ReplaceSectionText(const DocSelection& selection, const std::string_view& text) { ReplaceSectionText(selection.start, selection.end, text); }

	inline void ClearText() { setText(""); }

	inline bool IsEmpty() const { return document.isEmpty(); }
	inline size_t GetLineCount() const { return document.size(); }

	// render the text editor in a Dear ImGui context
	// note: if you overwrite windowFlags to for instance add ImGuiWindowFlags_NoSavedSettings
	// ensure you keep the default as they are required for the editor
	// - ImGuiWindowFlags_NoMove to ensure mouse drag event are passed to the editor
	// - ImGuiWindowFlags_HorizontalScrollbar to ensure a horizontal scrollbar is rendered when required
	inline bool Render(const char* title, const ImVec2& size=ImVec2(), ImGuiChildFlags childFlags=0, ImGuiWindowFlags windowFlags=ImGuiWindowFlags_NoMove | ImGuiWindowFlags_HorizontalScrollbar) { return render(title, size, childFlags, windowFlags); }

	// programmatically set focus on the editor
	inline void SetFocus() { focusOnEditor = true; }

	// clipboard actions
	inline void Cut() { if (!config.readOnly) cut(); }
	inline void Copy() const { copy(); }
	inline void Paste() { if (!config.readOnly) paste(); }
	inline void Undo() { if (!config.readOnly) undo(); }
	inline void Redo() { if (!config.readOnly) redo(); }
	inline bool CanUndo() const { return !config.readOnly && transactions.canUndo(); };
	inline bool CanRedo() const { return !config.readOnly && transactions.canRedo(); };
	inline size_t GetUndoIndex() const { return transactions.getUndoIndex(); };

	// manipulate cursors and selections (line numbers are zero-based)
	inline void SelectAll() { selectAll(); }
	inline void SelectLine(size_t line) { selectLine(normalizeLine(line)); }
	inline void SelectLines(size_t start, size_t end) { if (end < document.size() && start <= end) { selectLines(start, end); }}
	inline void SelectRegion(DocPos start, DocPos end) { selectRegion(normalizePos(start), normalizePos(end)); }
	inline void SelectToBrackets(bool includeBrackets=true) { selectToBrackets(includeBrackets); }
	inline void GrowSelections() { growSelections(); }
	inline void ShrinkSelections() { shrinkSelections(); }
	inline void AddNextOccurrence() { addNextOccurrence(); }
	inline void SelectAllOccurrences() { selectAllOccurrences(); }
	inline bool AnyCursorHasSelection() const { return cursors.anyHasSelection(); }
	inline bool AllCursorsHaveSelection() const { return cursors.allHaveSelection(); }
	inline bool CursorHasSelection(size_t cursor) const { return cursors.cursorHasSelection(cursor); }
	inline bool MainCursorHasSelection() const { return cursors.mainCursorHasSelection(); }
	inline bool CurrentCursorHasSelection() const { return cursors.currentCursorHasSelection(); }
	inline void ClearCursors() { cursors.clearAll(); }

	// get cursor positions (the meaning of main and current is explained in README.md)
	inline size_t GetNumberOfCursors() const { return cursors.size(); }
	inline DocPos GetCursorPosition(size_t cursor) const { return getCursorPosition(cursor); }
	inline DocPos GetMainCursorPosition() const { return getCursorPosition(cursors.getMainIndex()); }
	inline DocPos GetCurrentCursorPosition() const { return getCursorPosition(cursors.getCurrentIndex()); }
	inline DocSelection GetCursorSelection(size_t cursor) const { return getCursorSelection(cursor); }
	inline DocSelection GetMainCursorSelection() const { return getCursorSelection(cursors.getMainIndex()); }
	inline DocSelection GetCurrentCursorSelection() const { return getCursorSelection(cursors.getCurrentIndex()); }

	// get information at mouse position (e.g. from ImGui::GetMousePos())
	inline bool IsMousePosOverGlyph(const ImVec2& mousePos) const { return isMousePosOverGlyph(mousePos); }
	inline DocPos GetDocPosAtMousePos(const ImVec2& mousePos) const {return  getDocPosAtMousePos(mousePos); }
	inline std::string GetWordAtMousePos(const ImVec2& mousePos) const { return getWordAtMousePos(mousePos); }

	// scrolling support
	enum class Scroll {
		alignTop,
		alignMiddle,
		alignBottom
	};

	inline void ScrollToLine(size_t line, Scroll alignment) { scrollToLine(normalizeLine(line), alignment); }
	inline size_t GetFirstVisibleRow() const { return firstVisibleRow; }
	inline size_t GetLastVisibleRow() const { return lastVisibleRow; }
	inline size_t GetFirstVisibleColumn() const { return firstVisibleColumn; }
	inline size_t GetLastVisibleColumn() const { return lastVisibleColumn; }

	// specify a new cursor position and scroll to it (if required)
	// if the new position is currently in a folded region, it will be automatically unfolded
	inline void SetCursor(DocPos pos) { setCursor(normalizePos(pos)); }

	// note on setting scrolling and cursor position
	//
	// calling SetCursor or ScrollToLine has no effect until the next call to Render
	// this is because we can only do layout calculations when we are in a Dear ImGui drawing context
	// as a result, SetCursor or ScrollToLine just mark the request and let Render execute it
	//
	// the order of the calls is therefore important as they can interfere with each other
	// so if you call SetText, SetCursor and/or ScrollToLine before Render, the order should be:
	//
	// * call SetText first as it resets the entire editor state including cursors and scrolling
	// * then call SetCursor as it sets the cursor and requests that we make the cursor visible (i.e. scroll to it)
	// * then call ScrollToLine to mark the exact scroll location (it cancels the possible SetCursor scroll request)
	// * call Render to properly update the entire state
	//
	// this works while opening the editor as well as later

	// get glyph size in pixels
	inline float GetLineHeight() const { return glyphSize.y; }
	inline float GetGlyphWidth() const { return glyphSize.x; }

	// coordinate transformation
	inline VisPos DocPos2VisPos(DocPos pos) const { return docPos2VisPos(normalizePos(pos)); }
	inline DocPos VisPos2DocPos(VisPos pos) const { return visPos2DocPos(normalizePos(pos)); }

	// see if a specified document location is visible (not folded and currently on screen)
	inline bool IsDocPosVisible(DocPos pos) const { return isDocPosVisible(normalizePos(pos)); }

	// see if a visual position covers a glyph
	inline bool IsVisPosOverGlyph(VisPos pos) const { return typeSetter.isVisPosOverGlyph(normalizePos(pos)); }

	// find/replace support
	inline void SelectFirstOccurrenceOf(const std::string_view& text, bool caseSensitive=true, bool wholeWord=false) { selectFirstOccurrenceOf(text, caseSensitive, wholeWord); }
	inline void SelectNextOccurrenceOf(const std::string_view& text, bool caseSensitive=true, bool wholeWord=false) { selectNextOccurrenceOf(text, caseSensitive, wholeWord); }
	inline void SelectAllOccurrencesOf(const std::string_view& text, bool caseSensitive=true, bool wholeWord=false) { selectAllOccurrencesOf(text, caseSensitive, wholeWord); }
	inline void ReplaceTextInCurrentCursor(const std::string_view& text) { if (!config.readOnly) replaceTextInCurrentCursor(text); }
	inline void ReplaceTextInAllCursors(const std::string_view& text) { if (!config.readOnly) replaceTextInAllCursors(text); }

	inline void OpenFindReplaceWindow() { openFindReplace(); }
	inline void CloseFindReplaceWindow() { closeFindReplace(); }
	inline void SetFindButtonLabel(const std::string_view& label) { findButtonLabel = label; }
	inline void SetFindAllButtonLabel(const std::string_view& label) { findAllButtonLabel = label; }
	inline void SetReplaceButtonLabel(const std::string_view& label) { replaceButtonLabel = label; }
	inline void SetReplaceAllButtonLabel(const std::string_view& label) { replaceAllButtonLabel = label; }
	inline bool HasFindString() const { return findText.size(); }
	inline void FindNext() { findNext(); }
	inline void FindAll() { findAll(); }

	// access markers (line numbers are zero-based)
	// markers are attached to lines and are not effected by inserts or deletes before
	// if a line with a marker is deleted, undo doesn't restore it
	inline void AddMarker(size_t line, ImU32 lineNumberColor, ImU32 textColor, const std::string_view& lineNumberTooltip, const std::string_view& textTooltip) { addMarker(normalizeLine(line), lineNumberColor, textColor, lineNumberTooltip, textTooltip); }
	inline void ClearMarkers() { clearMarkers(); }
	inline bool HasMarkers() const { return markers.size() != 0; }

	// access squiggly underlines
	// squigglies are attached to glyphs and are not effected  by inserts or deletes before
	// if a glyph with a squiggle is deleted, undo doesn't restore it
	inline void AddSquiggle(DocPos start, DocPos end, size_t type, ImU32 color, const std::string_view& tooltip = std::string_view()) { addSquiggle(normalizePos(start), normalizePos(end), type, color, tooltip); }
	inline void ClearSquiggles(DocPos start, DocPos end) { clearSquiggles(normalizePos(start), normalizePos(end)); }
	inline void ClearSquiggles(size_t type) { clearSquiggles(type); }
	inline void ClearSquiggles() { clearSquiggles(); }
	inline bool HasSquiggles() const { return squiggles.size() != 0; }

	// specify a change callback (called when changes are made (including undo/redo))
	// the delay parameter specifies a time in miliseconds that the editor will wait for before calling
	// which helps in case you don't need to track every keystroke
	// passing nullptr for callback deactivates the feature
	inline void SetChangeCallback(std::function<void()> callback, int delay=0) {
		delayedChangeCallback = callback;
		delayedChangeDelay = std::chrono::milliseconds(delay);
	}

	// detailed change report passed to callback below
	// this callback is different from the one above as it reports every change (not just a summary) and is very detailed
	// the insert flag states whether the change was an insert (true) or a delete (false)
	// in case of an overwrite, there will be two actions (first a delete and then an insert)
	// the start parameters refer to the insert point or the start of the delete
	// the end parameters refer to the end of the inserted text or the end of the deleted text
	// the text parameter contains the inserted or deleted text
	// line and index values are zero-based
	struct Change {
		bool insert;
		DocPos start;
		DocPos end;
		std::string text;
	};

	// specify a transaction callback (live document changes in great detail)
	// it provides a list of changes made to the document in a single transaction (in the right order)
	// be carefull with this callback as it gets very verbose (called on every keystroke, delete, cut, paste, undo and redo)
	// passing nullptr deactivates the callback
	inline void SetTransactionCallback(std::function<void(const std::vector<Change>&)> callback) { transactions.setCallback(callback); }

	// line-based callbacks (line numbers are zero-based)
	// insertor callback is called when for each line inserted and the result is used as the new line specific user data
	// deletor callback is called for each line deleted (line specific user data is passed to callback)
	// setting either callback to nullptr will deactivate that callback
	inline void SetInsertor(std::function<void*(size_t line)> callback) { document.setInsertor(callback); }
	inline void SetDeletor(std::function<void(size_t line, void* data)> callback) { document.setDeletor(callback); }

	// line-based user data (line numbers are zero-based)
	// allowing integrators to associate external data with select lines or all lines
	// user data is an opaque void* that must be managed externally
	// user data is also passed to the decorator and popup callbacks (see below)
	// user data is attached to a line and insertions/deletions don't effect this
	// if a line with user data is removed, it won't come back on a redo
	// the deletor callback (if specified) is called when a line is deleted (see above)
	inline void SetUserData(size_t line, void* data) { document.setUserData(normalizeLine(line), data); }
	inline void* GetUserData(size_t line) const { return document.getUserData(normalizeLine(line)); }
	inline void IterateUserData(std::function<void(size_t line, void* data)> callback) const { document.iterateUserData(callback); }

	// line-based decoration
	struct Decorator {
		size_t line; // zero-based
		float width;
		float height;
		ImVec2 glyphSize;
		void* userData;
	};

	// setup a line decorator (width is number of glyphs)
	inline void SetLineDecorator(size_t width, std::function<void(Decorator& decorator)> callback) {
		decoratorWidth = width;
		decoratorCallback = callback;
	}

	inline void ClearLineDecorator() { SetLineDecorator(0, nullptr); }
	inline bool HasLineDecorator() const { return decoratorWidth != 0 && decoratorCallback != nullptr; }

	// custom text cursor (caret) rendering
	struct CustomCaret {
		// draw list to submit rendering commands to
		ImDrawList* drawList;

		// top left corner of glyph where cursor is (in screen coordinates)
		// can be used directly to submit drawing commands
		ImVec2 glyphPos;

		// visible size of glyph
		ImVec2 glyphSize;

		// flag indicating if cursor is visible (based on configuration and standard blinking algorithm)
		// this can be ignored if the custom caret has its own animation algorithm
		bool caretVisible;

		// color of cursor caret as per the current palette
		// that can also be ignored if custom caret has its own palette of animation
		ImU32 caretColor;

		// index of the cursor being rendered (in case additional cursor information is required)
		size_t cursorIndex;
	};

	inline void SetCustomCaretRenderer(std::function<void(const CustomCaret& caret)> callback) { customCaretCallback = callback; }
	inline void ClearCustomCaretRenderer() { customCaretCallback = nullptr; }
	inline bool HasCustomCaretRenderer() const { return customCaretCallback != nullptr; }

	// setup right click or hover callbacks
	// the editor sets up a popup menu in the right location
	// the callback has to populate it
	// context callbacks activate on a right click
	// hover callbacks are just based on position (no mouse buttons required)
	struct PopupData {
		DocPos pos;
		void* userData;
	};

	inline void SetLineNumberContextMenuCallback(std::function<void(PopupData& data)> callback) { lineNumberContextMenuCallback = callback; }
	inline void ClearLineNumberContextMenuCallback() { SetLineNumberContextMenuCallback(nullptr); }
	inline bool HasLineNumberContextMenuCallback() const { return lineNumberContextMenuCallback != nullptr; }

	inline void SetTextContextMenuCallback(std::function<void(PopupData& data)> callback) { textContextMenuCallback = callback; }
	inline void ClearTextContextMenuCallback() { SetTextContextMenuCallback(nullptr); }
	inline bool HasTextContextMenuCallback() const { return textContextMenuCallback != nullptr; }

	inline void SetTextHoverCallback(std::function<void(PopupData& data)> callback) { textHoverCallback = callback; }
	inline void ClearTextHoverCallback() { SetTextHoverCallback(nullptr); }
	inline bool HasTextHoverCallback() const { return textHoverCallback != nullptr; }

	// line folding support (only works when line folding is activated)
	inline void FoldAroundLine(size_t line) { if (config.lineFolding) { lineFold.foldAroundLine(document, normalizeLine(line)); } }
	inline void UnfoldAroundLine(size_t line) { if (config.lineFolding) { lineFold.unfoldAroundLine(document, normalizeLine(line)); } }
	inline void ToggleAtLine(size_t line) { if (config.lineFolding) { lineFold.toggleAtLine(document, normalizeLine(line)); } }
	inline void UnfoldAll() { if (config.lineFolding) { lineFold.unfoldAll(document); } }

	inline bool IsLineFoldable(size_t line) const { return isLineFoldable(normalizeLine(line)); }
	inline bool IsLineFolded(size_t line) const { return isLineFolded(normalizeLine(line)); }
	inline bool IsLineVisible(size_t line) const { return isLineVisible(normalizeLine(line)); }
	inline bool IsLineHidden(size_t line) const { return isLineHidden(normalizeLine(line)); }

	// useful functions to work on selections
	// NOTE: functions provided to FilterSelections or FilterLines should accept and return UTF-8 encoded strings
	inline void IndentLines() { if (!config.readOnly) indentLines(); }
	inline void DeindentLines() { if (!config.readOnly) deindentLines(); }
	inline void MoveUpLines() { if (!config.readOnly) moveUpLines(); }
	inline void MoveDownLines() { if (!config.readOnly) moveDownLines(); }
	inline void ToggleComments() { if (!config.readOnly && config.language) toggleComments(); }
	inline void FilterSelections(std::function<std::string(std::string_view)> filter) { if (!config.readOnly) filterSelections(filter); }
	inline void SelectionToLowerCase() { if (!config.readOnly) selectionToLowerCase(); }
	inline void SelectionToUpperCase() { if (!config.readOnly) selectionToUpperCase(); }

	// useful functions to work on entire text
	inline void StripTrailingWhitespaces() { if (!config.readOnly) stripTrailingWhitespaces(); }
	inline void FilterLines(std::function<std::string(std::string_view)> filter) { if (!config.readOnly) filterLines(filter); }
	inline void TabsToSpaces() { if (!config.readOnly) tabsToSpaces(); }
	inline void SpacesToTabs() { if (!config.readOnly) spacesToTabs(); }

	// color palette support
	enum class Color : char {
		text,
		keyword,
		declaration,
		number,
		string,
		punctuation,
		preprocessor,
		identifier,
		knownIdentifier,
		comment,
		background,
		cursor,
		selection,
		whitespace,
		matchingBracketBackground,
		matchingBracketActive,
		matchingBracketLevel1,
		matchingBracketLevel2,
		matchingBracketLevel3,
		matchingBracketError,
		lineNumber,
		currentLineNumber,
		count
	};

	struct Palette : public std::array<ImU32, static_cast<size_t>(Color::count)> {
		inline ImU32 get(Color color) const { return at(static_cast<size_t>(color)); }
	};

	inline void SetPalette(const Palette& newPalette) { paletteBase = newPalette; paletteAlpha = -1.0f; }
	inline const Palette& GetPalette() const { return paletteBase; }
	static inline void SetDefaultPalette(const Palette& aValue) { defaultPalette = aValue; }
	static inline Palette& GetDefaultPalette() { return defaultPalette; }

	static const Palette& GetDarkPalette();
	static const Palette& GetLightPalette();

	// line break options
	enum class BreakOption : char {
		mustBreak,
		allowBreak,
		noBreak,
		undefined
	};

	// a single colored character (a glyph)
	struct Glyph {
		// constructors
		Glyph() = default;
		Glyph(ImWchar cp) : codepoint(cp) {}
		Glyph(ImWchar cp, Color col) : codepoint(cp), color(col) {}

		// unicode codepoint for this glyph
		ImWchar codepoint = 0;

		// color for this glyph if a language is specified
		// maintained by the Colorizer and the Bracketeer overlays
		Color color = Color::text;

		// maintained by the TypeSetter overlay
		BreakOption breakOption = BreakOption::undefined;

		// squiggle reference
		size_t squiggle = 0;
	};

	// iterator used in language-specific tokenizers
	// this iterator points to unicode codepoints
	class Iterator {
	public:
		// constructors
		Iterator() = default;
		Iterator(Glyph* g) : glyph(g) {}

		using iterator_category = std::forward_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = ImWchar;
		using pointer = ImWchar*;
		using reference = ImWchar&;

		inline reference operator*() const { return glyph->codepoint; }
		inline pointer operator->() const { return &(glyph->codepoint); }
		inline Iterator& operator++() { glyph++; return *this; }
		inline Iterator operator++(int) { Iterator tmp = *this; glyph++; return tmp; }
		inline size_t operator-(const Iterator& a) { return static_cast<size_t>(glyph - a.glyph); }
		inline friend bool operator==(const Iterator& a, const Iterator& b) { return a.glyph == b.glyph; };
		inline friend bool operator!=(const Iterator& a, const Iterator& b) { return !(a.glyph == b.glyph); };
		inline friend bool operator<(const Iterator& a, const Iterator& b) { return a.glyph < b.glyph; };
		inline friend bool operator<=(const Iterator& a, const Iterator& b) { return a.glyph <= b.glyph; };
		inline friend bool operator>(const Iterator& a, const Iterator& b) { return a.glyph > b.glyph; };
		inline friend bool operator>=(const Iterator& a, const Iterator& b) { return a.glyph >= b.glyph; };

	private:
		// properties
		Glyph* glyph;
	};

	// language support
	struct Language {
		// name of the language
		std::string name;

		// flag to describe if keywords and identifiers are case sensitive (which is the default)
		bool caseSensitive = true;

		// the character that starts a preprocessor directive (can be 0 if language doesn't have this feature)
		ImWchar preprocess = 0;

		// a character sequence that start a single line comment (can be blank if language doesn't have this feature)
		std::string singleLineComment;

		// an alternate single line comment character sequence (can be blank if language doesn't have this feature)
		std::string singleLineCommentAlt;

		// the start and end character sequence for multiline comments (can be blank language doesn't have this feature)
		std::string commentStart;
		std::string commentEnd;

		// functions to help tokenize multilevel, multiline comments (can be nullptr if language doesn't have this feature)
		// start and end refer to the characters being tokenized
		// functions should return an iterator to the character after the detected token and set level
		// returning start means no token was found
		std::function<Iterator(Iterator start, Iterator end, size_t& level)> commentLevelStart;
		std::function<Iterator(Iterator start, Iterator end, size_t& level)> commentLevelEnd;

		// flags specifying whether language supports single quoted ['] and/or double quoted [""] strings
		bool hasSingleQuotedStrings = false;
		bool hasDoubleQuotedStrings = false;

		// other character sequences that start and end strings (can be blank if language doesn't have this feature)
		std::string otherStringStart;
		std::string otherStringEnd;

		// alternate character sequences that start and end strings (can be blank if language doesn't have this feature)
		std::string otherStringAltStart;
		std::string otherStringAltEnd;

		// character inside string used to escape the next character (can be 0 if language doesn't have this feature)
		ImWchar stringEscape = 0;

		// functions to help tokenize multilevel, multiline strings (can be nullptr if language doesn't have this feature)
		// start and end refer to the characters being tokenized
		// functions should return an iterator to the character after the detected token and set level
		// returning start means no token was found
		std::function<Iterator(Iterator start, Iterator end, size_t& level)> stringLevelStart;
		std::function<Iterator(Iterator start, Iterator end, size_t& level)> stringLevelEnd;

		// does the language use indentation for blocks (e.g Python)
		bool indentationForBlocks = false;

		// set of keywords, declarations, identifiers used in the language (can be blank if language doesn't have these features)
		// if language is not case sensitive, all entries should be in lower case

		// guidance for these categories
		// 1. these categories refer to different colors in the color palette
		// 2. keywords are typically used to highlight control/reserved words in a language (palette color keyword)
		// 3. declarations are used in strongly typed languages to highlight builtin types or the keywords to create a type (palette color declaration)
		// 4. identifiers are used to color the language predefined variables  (palette color knownIdentifier) differently from the user variables (palette color identifier)
		std::unordered_set<std::string> keywords;
		std::unordered_set<std::string> declarations;
		std::unordered_set<std::string> identifiers;

		// function to determine if specified character in considered punctuation
		std::function<bool(ImWchar)> isPunctuation;

		// functions to tokenize identifiers and numbers (can be nullptr if language doesn't have this feature)
		// start and end refer to the characters being tokenized
		// functions should return an iterator to the character after the detected token
		// returning start means no token was found
		std::function<Iterator(Iterator start, Iterator end)> getIdentifier;
		std::function<Iterator(Iterator start, Iterator end)> getNumber;

		// function to implement custom tokenizer (can be nullptr if language doesn't have this feature)
		// if a token is found, function should return an iterator to the character after the token and set the color
		std::function<Iterator(Iterator start, Iterator end, Color& color)> customTokenizer;

		// predefined language definitions
		static const Language* C();
		static const Language* Cpp();
		static const Language* Cs();
		static const Language* AngelScript();
		static const Language* Lua();
		static const Language* Python();
		static const Language* Glsl();
		static const Language* Hlsl();
		static const Language* Json();
		static const Language* Markdown();
		static const Language* Sql();
	};

	void SetLanguage(const Language* language);
	inline const Language* GetLanguage() const { return config.language; };
	inline bool HasLanguage() const { return config.language != nullptr; }
	inline std::string GetLanguageName() const { return config.language == nullptr ? "None" : config.language->name; }
	inline void SetLanguageChangeCallback(std::function<void()> callback) { languageChangeCallback = callback; }

	// iterate through identifiers detected by the colorizer (based on current language)
	inline void IterateIdentifiers(std::function<void(const std::string& identifier)> callback) const { document.iterateIdentifiers(callback); }

	// autocomplete state (acts as API between editor and outer application)
	struct AutoCompleteState {
		// current context
		std::string searchTerm;
		DocPos searchTermStart;
		DocPos searchTermEnd;

		bool inIdentifier;
		bool inNumber;
		bool inComment;
		bool inString;

		// currently selected language (could be nullptr if no language is selected)
		const Language* language;

		// optional opaque void* provided by app when autocomplete was setup
		void* userData;

		// auto complete suggestions te be provided by app callback (the app is responsible for sorting)

		// the editor does not automatically include language specific keywords or identifiers in the suggestion list
		// this is left to the application so it can be context specific in case a language server is used
		// a pointer to the current language definition is provided so callbacks have easy access
		std::vector<std::string> suggestions;

		// set this to true if you are building the suggestion list asynchronously and provide it later
		// this way autocomplete is not cancelled if the suggestion list is empty and the user hits tab or enter
		bool suggestionsPromise = false;
	};

	// autocomplete configuration (defaults are like Visual Studio Code)
	struct AutoCompleteConfig {
		// specifies whether typing by the user triggers autocomplete
		bool triggerOnTyping = true;

		// specifies whether the specified shortcut triggers autocomplete
		bool triggerOnShortcut = true;

		// specifies whether typing (or shortcut) in comments or strings triggers autocomplete
		bool triggerInComments = false;
		bool triggerInStrings = false;

		// manual trigger key sequence (default is Ctrl+space on all platforms, even MacOS)
		// remember Dear ImGui reverses Ctrl and Command on MacOS
		#if __APPLE__
			ImGuiKeyChord triggerShortcut = ImGuiMod_Super | ImGuiKey_Space;
		#else
			ImGuiKeyChord triggerShortcut = ImGuiMod_Ctrl | ImGuiKey_Space;
		#endif

		// see if single suggestions are automatically inserted
		// this only works when triggered manually
		bool autoInsertSingleSuggestions = false;

		// delay in milliseconds between autocomplete trigger and suggestions popup
		std::chrono::milliseconds triggerDelay{200};

		// text label used when no suggestions are available (this allows for internationalization)
		std::string noSuggestionsLabel = "No suggestions";

		// width of suggestion popup expressed in number of glyphs
		size_t suggestionWidth = 30;

		// called when autocomplete is configured, active and the editor needs an updated suggestions list
		// callback must populate and order suggestions in state object
		// suggestion list is not cleared by editor between callbacks
		// callback is called during the rendering loop (so don't take too long)

		// if it takes too long, applications should do search in separate thread and
		// use API to report results (see SetAutoCompleteSuggestions)
		// callback should set suggestionsPromise to true in this case
		std::function<void(AutoCompleteState&)> callback;

		// opaque void* that must be managed externally but passed to callback
		void* userData = nullptr;
	};

	// configure and activate autocomplete (passing nullptr deactivates it)
	inline void SetAutoCompleteConfig(const AutoCompleteConfig* autoCompleteConfig) { autocomplete.setConfig(autoCompleteConfig); }

	// provide autocomplete suggestions asynchronously (in case a callback takes to long and lookup is handled in a separate thread/process)
	// this call is not threadsafe and must be called from the rendering thread (you must synchronize with your lookup thread yourself)
	inline void SetAutoCompleteSuggestions(const std::vector<std::string>& suggestions) { autocomplete.setSuggestions(suggestions); }

	// support functions for unicode codepoints
	struct CodePoint {
		static std::string_view::const_iterator skipBOM(std::string_view::const_iterator i, std::string_view::const_iterator end);
		static std::string_view::const_iterator read(std::string_view::const_iterator i, std::string_view::const_iterator end, ImWchar* codepoint);
		static size_t write(char* i, ImWchar codepoint); // must point to buffer of 4 characters (returns number of characters written)
		static bool isLetter(ImWchar codepoint);
		static bool isNumber(ImWchar codepoint);
		static bool isWord(ImWchar codepoint);
		static bool isWhiteSpace(ImWchar codepoint);
		static bool isXidStart(ImWchar codepoint);
		static bool isXidContinue(ImWchar codepoint);
		static bool isLower(ImWchar codepoint);
		static bool isUpper(ImWchar codepoint);
		static bool isEastAsian(ImWchar codepoint);
		static ImWchar toUpper(ImWchar codepoint);
		static ImWchar toLower(ImWchar codepoint);

		static constexpr ImWchar singleQuote = '\'';
		static constexpr ImWchar doubleQuote = '"';
		static constexpr ImWchar openCurlyBracket = '{';
		static constexpr ImWchar closeCurlyBracket = '}';
		static constexpr ImWchar openSquareBracket = '[';
		static constexpr ImWchar closeSquareBracket = ']';
		static constexpr ImWchar openParenthesis = '(';
		static constexpr ImWchar closeParenthesis = ')';

		static inline bool isPairOpener(ImWchar ch) {
			return
				ch == openCurlyBracket ||
				ch == openSquareBracket ||
				ch == openParenthesis ||
				ch == singleQuote ||
				ch == doubleQuote;
		}

		static inline bool isPairCloser(ImWchar ch) {
			return
				ch == closeCurlyBracket ||
				ch == closeSquareBracket ||
				ch == closeParenthesis ||
				ch == singleQuote ||
				ch == doubleQuote;
		}

		static inline ImWchar toPairCloser(ImWchar ch) {
			return
				(ch == openCurlyBracket) ? closeCurlyBracket :
				(ch == openSquareBracket) ? closeSquareBracket :
				(ch == openParenthesis) ? closeParenthesis:
				ch;
		}

		static inline ImWchar toPairOpener(ImWchar ch) {
			return
				(ch == closeCurlyBracket) ? openCurlyBracket :
				(ch == closeSquareBracket) ? openSquareBracket :
				(ch == closeParenthesis) ? openParenthesis:
				ch;
		}

		static inline bool isMatchingPair(ImWchar open, ImWchar close) {
			return isPairOpener(open) && close == toPairCloser(open);
		}

		static inline bool isBracketOpener(ImWchar ch) {
			return
				ch == openCurlyBracket ||
				ch == openSquareBracket ||
				ch == openParenthesis;
		}

		static inline bool isBracketCloser(ImWchar ch) {
			return
				ch == closeCurlyBracket ||
				ch == closeSquareBracket ||
				ch == closeParenthesis;
		}

		static inline bool isMatchingBrackets(ImWchar open, ImWchar close) {
			return isBracketOpener(open) && close == toPairCloser(open);
		}
	};

	// configuration for line break algorithm used when word wrap is active
	struct LineBreakConfig {
		// wrap mode (false = simple mode, true = unicode line break mode)
		bool useUnicodeAnnex14 = false;

		// simple mode options (strings of UTF-8 encoded glyphs)
		std::string breakAfter = " \t{[(";
		std::string breakBefore = ".";

		// unicode line breaking options
		// based on the unicode standard annex #14 which identifies break
		// opportunities expressed as rules which can be (de)activated below
		// see https://www.unicode.org/reports/tr14 for details
		bool lb2 = true;
		bool lb3 = true;
		bool lb4 = true;
		bool lb5 = true;
		bool lb6 = true;
		bool lb7 = true;
		bool lb8 = true;
		bool lb8a = true;
		bool lb9 = true;
		bool lb10 = true;
		bool lb11 = true;
		bool lb12 = true;
		bool lb12a = true;
		bool lb13 = true;
		bool lb14 = true;
		bool lb15a = true;
		bool lb15b = true;
		bool lb15c = true;
		bool lb15d = true;
		bool lb16 = true;
		bool lb17 = true;
		bool lb18 = true;
		bool lb19 = true;
		bool lb19a = true;
		bool lb20 = true;
		bool lb20a = true;
		bool lb21 = true;
		bool lb21a = true;
		bool lb21b = true;
		bool lb22 = true;
		bool lb23 = true;
		bool lb23a = true;
		bool lb24 = true;
		bool lb25 = true;
		bool lb26 = true;
		bool lb27 = true;
		bool lb28 = true;
		bool lb28a = true;
		bool lb29 = true;
		bool lb30 = true;
		bool lb30a = true;
		bool lb30b = true;
	};

	// set the line break configuration
	inline void SetLineBreakConfig(LineBreakConfig& newConfig) { typeSetter.setLineBreakConfig(newConfig); }

	// set the current ImGui context
	//
	// this is ONLY necessary if you are compiling this widget as a DLL (which is NOT recommended)
	// it sets the global variable GImGui, which is not shared across DLL boundaries
	// see GImGui documentation in imgui.cpp for more details
	static inline void SetImGuiContext(ImGuiContext* ctx) { ImGui::SetCurrentContext(ctx); }

protected:
	//
	// below is the private API
	// private members (functions and variables) start with a lowercase character
	// private type names start with a uppercase character
	//

	// everybody needs a friend
	friend class TextDiff;

	// editor configuration
	struct Config {
		// options
		size_t tabSize = 4;
		bool insertSpacesOnTabs = false;
		float lineSpacing = 1.0f;
		bool wordWrap = false;
		bool readOnly = false;
		bool caretsVisible = true;
		bool autoIndent = true;
		bool showSpaces = true;
		bool showTabs = true;
		bool showLineNumbers = true;
		bool showMiniMap = false;
		size_t miniMapColumns = 0;
		bool showScrollbarMiniMap = true;
		bool showMatchingBrackets = true;
		bool completePairedGlyphs = true;
		bool lineFolding = false;
		bool overwrite = false;
		bool panMode = true;
		bool showPanScrollIndicator = true;
		size_t leftMargin = 1; // margins are expressed in number of glyphs
		size_t decorationMargin = 1;
		size_t textMargin = 2;

		// language support
		const Language* language = nullptr;

		// word wrap limits
		size_t wordWrapColumns = 0;
	} config;

	// colorizer/tokenizer state
	enum class LineState : char {
		inText,
		inComment,
		inCommentLevel1,
		inCommentLevel2,
		inCommentLevel3,
		inCommentLevel4,
		inCommentLevel5,
		inCommentLevel6,
		inCommentLevel7,
		inSingleQuotedString,
		inDoubleQuotedString,
		inOtherString,
		inOtherStringAlt,
		inStringLevel0,
		inStringLevel1,
		inStringLevel2,
		inStringLevel3,
		inStringLevel4,
		inStringLevel5,
		inStringLevel6,
		inStringLevel7
	};

	static inline bool lineStateInComment(LineState state) { return state >= LineState::inComment && state <= LineState::inCommentLevel7; }
	static inline bool lineStateInString(LineState state) { return state >= LineState::inSingleQuotedString && state <= LineState::inStringLevel7; }
	static inline bool lineStateInStringLevel(LineState state) { return state >= LineState::inStringLevel0 && state <= LineState::inStringLevel7; }
	static inline LineState commentLevelToLineState(size_t level) { return static_cast<LineState>(static_cast<int>(LineState::inComment) + level); }
	static inline LineState stringLevelToLineState(size_t level) { return static_cast<LineState>(static_cast<int>(LineState::inStringLevel0) + level); }
	static constexpr size_t maxCommentLevel = static_cast<int>(LineState::inCommentLevel7) - static_cast<int>(LineState::inComment);
	static constexpr size_t maxStringLevel = static_cast<int>(LineState::inStringLevel7) - static_cast<int>(LineState::inStringLevel0);

	// line folding state
	enum class FoldingState : char {
		foldable,
		folded,
		visible,
		hidden
	};

	// information for wrapped lines
	struct LineSection {
		LineSection() = default;

		LineSection(size_t startIndex, size_t endIndex, size_t columns, size_t indent) :
			startIndex(startIndex),endIndex(endIndex), columns(columns), indent(indent) {}

		size_t startIndex;
		size_t endIndex;
		size_t columns;
		size_t indent;
	};

	using LineSections = std::vector<LineSection>;

	// a single line in a document
	struct Line : public std::vector<Glyph> {
		// line indent in number of columns maintained by Document
		size_t indent = 0;

		// color state maintained by the Colorizer overlay
		bool needsColorizing = true;
		LineState state = LineState::inText;

		// line folding state maintained by the LineFold overlay
		FoldingState foldingState = FoldingState::visible;

		// typesetting state maintained by the TypeSetter overlay
		bool needsTypeSetting = true;
		size_t row = 0;
		size_t rows = 0;
		size_t columns = 0;
		std::shared_ptr<LineSections> sections;

		// marker reference (0 means no marker for this line)
		size_t marker = 0;

		// user data associated with this line
		void* userData = nullptr;
	};


	// the document being edited (Lines of Glyphs)
	class Document : public std::vector<Line> {
	public:
		// constructor
		Document() { emplace_back(); }

		// manipulate document text (strings should be UTF-8 encoded)
		void setText(const Config& config, const std::string_view& text);
		void setText(const Config& config, const std::vector<std::string_view>& text);
		DocPos insertText(const Config& config, DocPos start, const std::string_view& text);
		void deleteText(const Config& config, DocPos start, DocPos end);

		// access document text (strings are UTF-8 encoded)
		std::string getText() const;
		std::string getLineText(size_t line) const;
		std::string getSectionText(DocPos start, DocPos end) const;
		ImWchar getCodePoint(DocPos location) const;

		// iterate through glyphs between two positions
		void iterateGlyphs(DocPos start, DocPos end, std::function<void(Glyph& glyph)> callback);

		// get line or color state
		inline LineState getLineState(size_t line) const { return at(line).state; }
		Color getColor(DocPos location) const;

		// see if document is empty
		inline bool isEmpty() const { return size() == 1 && at(0).size() == 0; }

		// coordinate operations in context of document
		DocPos getLeft(DocPos from, bool wordMode=false) const;
		DocPos getRight(DocPos from, bool wordMode=false) const;
		DocPos getTop() const;
		DocPos getBottom() const;
		DocPos getStartOfLine(DocPos from) const;
		DocPos getEndOfLine(DocPos from) const;
		inline DocPos getNextLine(DocPos from) const { return getRight(getEndOfLine(from)); }

		// search in document
		DocPos findWordStart(DocPos from, bool wordOnly=false) const;
		DocPos findWordEnd(DocPos from, bool wordOnly=false) const;
		bool findText(DocPos from, const std::string_view& text, bool caseSensitive, bool wholeWord, DocPos& start, DocPos& end) const;

		// see if document was updated this frame
		inline bool isUpdated() const { return updated; }
		inline void resetUpdated() { updated = false; }

		// line-based callbacks
		inline void setInsertor(std::function<void*(size_t line)> callback) { insertor = callback; }
		inline void setDeletor(std::function<void(size_t line, void* data)> callback) { deletor = callback; }

		// access line user data
		void setUserData(size_t line, void* data);
		void* getUserData(size_t line) const;
		void iterateUserData(std::function<void(size_t line, void* data)> callback) const;

		// iterate through document to find identifiers
		void iterateIdentifiers(std::function<void(const std::string& identifier)> callback) const;

		// utility functions
		bool isWholeWord(DocPos start, DocPos end) const;
		inline bool isEndOfLine(DocPos from) const { return from.index == at(from.line).size(); }
		inline bool isLastLine(size_t line) const { return line == size() - 1; }
		DocPos findPreviousNonWhiteSpace(DocPos from, bool includeEndOfLine=true) const;
		DocPos findNextNonWhiteSpace(DocPos from, bool includeEndOfLine=true) const;
		DocPos normalizePos(DocPos pos) const;
		inline size_t normalizeLine(size_t line) const { return line >= size() ? size() - 1 : line; }

		DocPos getLeftBeforeHiddenLines(DocPos pos) const;
		DocPos getRightAfterHiddenLines(DocPos pos) const;

	private:
		bool updated = false;

		std::function<void*(size_t)> insertor;
		std::function<void(size_t, void*)> deletor;

		void appendLine();
		void insertLine(size_t offset);
		void deleteLines(size_t start, size_t end);
		void clearDocument();
		void updateIndents(const Config& config, size_t start, size_t end);
	} document;

	// a single cursor
	class Cursor {
	public:
		// constructors
		Cursor() = default;
		Cursor(DocPos position) : start(position), end(position) {}
		Cursor(DocPos start, DocPos end) : start(start), end(end) {}

		// update the cursor
		inline void update(DocPos position) { end = position; updated = true; }
		inline void update(DocPos s, DocPos e) { start = s; end = e; updated = true; }
		inline void update(DocPos position, bool keep) { if (keep) update(position); else update(position, position); updated = true; }

		// grow the cursor either to the left or the right
		void grow(DocPos position);

		// adjust cursor for insert/delete operations
		// (these functions assume that insert or delete points are before the cursor)
		void adjustForInsert(DocPos insertStart, DocPos insertEnd);
		void adjustForDelete(DocPos deleteStart, DocPos deleteEnd);

		// ensure cursor is not on hidden line
		void ensureNotHidden(const Document& document);

		// access cursor properties
		inline DocPos getInteractiveStart() const { return start; }
		inline DocPos getInteractiveEnd() const { return end; }
		inline DocPos getSelectionStart() const { return start < end ? start : end; }
		inline DocPos getSelectionEnd() const { return start > end ? start : end; }
		inline bool hasSelection() const { return start != end; }

		inline void resetToStart() { update(getSelectionStart(), getSelectionStart()); }
		inline void resetToEnd() { update(getSelectionEnd(), getSelectionEnd()); }

		inline void setMain(bool value) { main = value; }
		inline bool isMain() const { return main; }

		inline void setCurrent(bool value) { current = value; }
		inline bool isCurrent() const { return current; }

		inline void setUpdated(bool value) { updated = value; }
		inline bool isUpdated() const { return updated; }

		inline void setPreferredColumn(size_t column) { preferredColumn = column; }
		inline size_t getPreferredColumn() const { return preferredColumn; }

	private:
		// helper functions
		static DocPos adjustCoordinateForInsert(DocPos position, DocPos insertStart, DocPos insertEnd);
		static DocPos adjustCoordinateForDelete(DocPos position, DocPos deleteStart, DocPos deleteEnd);

		// properties
		DocPos start;
		DocPos end;
		bool main = false;
		bool current = true;
		bool updated = true;

		// only used during up/down keyboard navigation
		size_t preferredColumn = 0;
	};

	// overlay to track the current list of cursors
	class Cursors : public std::vector<Cursor> {
	public:
		// constructor
		Cursors() { clearAll(); }

		// reset the cursors
		void reset();

		// erase all cursors and specify a new one
		inline void setCursor(DocPos position) { setCursor(position, position); }
		void setCursor(DocPos start, DocPos end);

		// add a cursor to the list
		inline void addCursor(DocPos c) { addCursor(c, c); }
		void addCursor(DocPos cursorStart, DocPos cursorEnd);

		// update the current cursor (the one added last)
		inline void updateCurrentCursor(DocPos position) { at(current).update(position); }
		inline void updateCurrentCursor(DocPos start, DocPos end) { at(current).update(start, end); }
		inline void updateCurrentCursor(DocPos position, bool keep) { at(current).update(position, keep); }

		// grow the current cursor either to the left or the right
		inline void growCurrentCursor(DocPos position) { at(current).grow(position); }

		// check cursor status
		inline bool hasMultiple() const { return size() > 1; }
		bool anyHasSelection() const;
		bool allHaveSelection() const;
		inline bool cursorHasSelection(size_t cursor) const { return cursor < size() ? at(cursor).hasSelection() : false; }
		inline bool mainCursorHasSelection() const { return at(main).hasSelection(); }
		inline bool currentCursorHasSelection() const { return at(current).hasSelection(); }
		inline bool mainHasUpdate() const { return at(main).isUpdated(); }
		bool anyHasUpdate() const;

		// clear the selections and create the default cursor
		void clearAll();

		// clear all additional cursors
		void clearAdditional(bool reset=false);

		// clear all updated flags
		void clearUpdated();

		// get main/current cursor
		inline Cursor& getMain() { return at(main); }
		inline size_t getMainIndex() const { return main; }
		inline Cursor& getCurrent() { return at(current); }
		inline size_t getCurrentIndex() const { return current; }
		inline iterator getMainAsIterator() { return begin() + static_cast<Iterator::difference_type>(main); }
		inline iterator getCurrentAsIterator() { return begin() + static_cast<Iterator::difference_type>(current); }

		// update cursors
		void update(const Document& document);

		// adjust cursors for insert/delete operations
		void adjustForInsert(iterator start, DocPos insertStart, DocPos insertEnd, bool includeCurrent=false);
		void adjustForDelete(iterator start, DocPos deleteStart, DocPos deleteEnd, bool includeCurrent=false);

	private:
		size_t main = 0;
		size_t current = 0;
	} cursors;

	// single action to be performed on the document as part of a larger transaction
	struct Action {
		// action types
		enum class Type : char {
			insertText,
			deleteText
		};

		// constructors
		Action() = default;
		Action(Type t, DocPos s, DocPos e, const std::string_view& txt) : type(t), start(s), end(e), text(txt) {}

		// properties
		Type type;
		DocPos start;
		DocPos end;
		std::string text;
	};

	// a collection of actions for a complete transaction
	class Transaction : public std::vector<Action> {
	public:
		// access state before/after transactions
		inline void setBeforeState(const Cursors& cursors) { before = cursors; }
		inline const Cursors& getBeforeState() const { return before; }
		inline void setAfterState(const Cursors& cursors) { after = cursors; }
		inline const Cursors& getAfterState() const { return after; }

		// add actions by type
		void addInsert(DocPos start, DocPos end, std::string_view text) { emplace_back(Action::Type::insertText, start, end, text); };
		void addDelete(DocPos start, DocPos end, std::string_view text) { emplace_back(Action::Type::deleteText, start, end, text); };

	private:
		// properties
		Cursors before;
		Cursors after;
	};

	// overlay for managing the transaction list to support do/undo/redo
	class Transactions : public std::vector<std::shared_ptr<Transaction>> {
	public:
		// reset the transactions
		void reset();

		// create a new transaction
		static inline std::shared_ptr<Transaction> create() { return std::make_shared<Transaction>(); }

		// add a transaction to the list and make it undoable
		void add(std::shared_ptr<Transaction> transaction);

		// undo the last transaction
		void undo(const Config& config, Document& document, Cursors& cursors);

		// redo the last undone transaction;
		void redo(const Config& config, Document& document, Cursors& cursors);

		// get status information
		inline size_t getUndoIndex() const { return undoIndex; }
		inline bool canUndo() const { return undoIndex > 0; }
		inline bool canRedo() const { return undoIndex < size(); }
		inline size_t getVersion() const { return version; }

		// set transaction callback
		inline void setCallback(std::function<void(const std::vector<Change>&)> cb) { callback = cb; }

	private:
		size_t undoIndex = 0;
		size_t version = 0;
		std::function<void(std::vector<Change>&)> callback;
	} transactions;

	// overlay to colorizer text (based on language tokenizing)
	class Colorizer {
	public:
		// update state (if required)
		bool update(const Config& config, Document& document);

	private:
		// update color in a single line
		LineState updateLine(Line& line);

		// see if string matches part of line
		static bool matches(Line::iterator start, Line::iterator end, const std::string_view& text);

		// set color for specified range of glyphs
		static inline void setColor(Line::iterator start, Line::iterator end, Color color) {
			while (start < end) (start++)->color = color;
		}

		// current state
		const Language* language = nullptr;
	} colorizer;

	// overlay to manage details about bracketed text
	struct BracketPair {
		BracketPair(ImWchar startChar, DocPos start, ImWchar endChar, DocPos end, size_t level, bool visible) :
			startChar(startChar), start(start), endChar(endChar), end(end), level(level), visible(visible) {}

		BracketPair(DocPos start, DocPos end) : startChar(0), start(start), endChar(0), end(end), level(0), visible(false) {}

			ImWchar startChar;
		DocPos start;
		ImWchar endChar;
		DocPos end;
		size_t level;
		bool visible;

		inline bool isAfter(DocPos location) const { return start > location; }
		inline bool isAround(DocPos location) const { return start < location && end >= location; }

		inline bool operator ==(const BracketPair& rhs) const { return start == rhs.start && end == rhs.end; }
	};

	// overlay for matching brackets (visible and invisible)
	// invisible brackets are for languages who's blocks are indent based (e.h. Python)
	class Bracketeer : public std::vector<BracketPair> {
	public:
		// update state (if required)
		void update(const Config& config, Document& document);

		// find relevant brackets
		const_iterator getEnclosingBrackets(DocPos location) const;
		const_iterator getEnclosingBrackets(DocPos first, DocPos last) const;
		const_iterator getInnerBrackets(DocPos first, DocPos last) const;

		// see if bracketeer was updated this frame
		inline bool isUpdated() const { return updated; }
		inline void resetUpdated() { updated = false; }

		// utility functions
		static inline bool isBracketCandidate(const Glyph& glyph) {
			return glyph.color == Color::punctuation ||
				glyph.color == Color::matchingBracketLevel1 ||
				glyph.color == Color::matchingBracketLevel2 ||
				glyph.color == Color::matchingBracketLevel3 ||
				glyph.color == Color::matchingBracketError;
		}

	private:
		bool showMatchingBrackets = false;
		const Language* language = nullptr;
		bool updated = false;
	} bracketeer;

	// captured a single line fold opportunity
	struct Fold {
		Fold() = default;
		Fold(size_t start, size_t end) : start(start), end(end) {}
		size_t start;
		size_t end;
	};

	// overlay for managing line folding
	class LineFold : public std::vector<Fold> {
	public:
		// update state (if required)
		bool update(const Config& config, Document& document, const Bracketeer& bracketeer);

		// (un)fold line
		void foldAroundLine(Document& document, size_t line);
		void unfoldAroundLine(Document& document, size_t line);
		void toggleAtLine(Document& document, size_t line);
		void unfoldAll(Document& document);

		static inline bool isFoldable(const Document& document, size_t line) {return document[line].foldingState == FoldingState::foldable; }
		static inline bool isFolded(const Document& document, size_t line) {return document[line].foldingState == FoldingState::folded; }
		static inline bool isVisible(const Document& document, size_t line) {return document[line].foldingState == FoldingState::visible; }
		static inline bool isHidden(const Document& document, size_t line) {return document[line].foldingState == FoldingState::hidden; }

		// see if line folding was updated this frame
		inline bool isUpdated() const { return updated; }
		inline void resetUpdated() { updated = false; }

	private:
		// properties
		bool lineFolding = false;
		bool updated = false;
		bool forceUpdate = true;
	} lineFold;

	// break lines when word wrap is active
	struct LineBreak {
		// classify glyphs on a line by applying line breaking rule
		void classify(Line& line);

		// configuration
		LineBreakConfig config;
		bool updateSets = true;
		std::unordered_set<ImWchar> breakAfter;
		std::unordered_set<ImWchar> breakBefore;
		static void updateSet(std::unordered_set<ImWchar>& set, std::string_view text);
	};

	// class representing a single visible row
	struct Row {
		Row() = default;
		Row(size_t line, size_t section, size_t columns) : line(line), section(section), columns(columns) {}
		size_t line;
		size_t section;
		size_t columns;
	};

	// overlay for mapping a logical document structure to a visual layout
	class TypeSetter : public std::vector<Row> {
	public:
		// update state (if required)
		bool update(const Config& config, Document& document, const LineFold& lineFold);

		// convert coordinates
		VisPos docPos2VisPos(const Document& document, DocPos pos) const;
		DocPos visPos2DocPos(const Document& document, VisPos pos) const;
		void screenPos2DocPos(const Document& document, ImVec2 screenPos, DocPos& glyphPos, DocPos& cursorPos) const;

		// normalize
		VisPos normalizePos(VisPos pos) const;

		// see if position is over text
		bool isVisPosOverGlyph(VisPos pos) const;

		// get information
		inline size_t getRowCount() const { return totalRows; }
		inline size_t getColumnCount() const { return totalColumns; }

		// set line break configuration
		inline void setLineBreakConfig(LineBreakConfig& lineBreakConfig) {
			lineBreak.config = lineBreakConfig;
			lineBreak.updateSets = true;
		}

		// see if type setter was updated this frame
		inline bool isUpdated() const { return updated; }
		inline void resetUpdated() { updated = false; }

	private:
		// update layout of specified line
		void updateLine(Line& line);

		// wrap line and count number of visible rows
		void wrapLine(Line& line);

		// current state
		size_t tabSize = 0;
		bool wordWrap = false;
		size_t wordWrapColumns = 0;
		bool updated = false;

		size_t totalRows = 0;
		size_t totalColumns = 0;

		// support for line break calculations
		LineBreak lineBreak;
	} typeSetter;

	// overlay with layout details for a full minimap
	struct MiniMap {
		// update state (if required)
		bool update(const Config& config, const Document& document, const TypeSetter& typeSetter);

		// properties
		struct Section {
			Section(size_t start, size_t end, Color color) : start(start), end(end), color(color) {}
			size_t start;
			size_t end;
			Color color;
		};

		struct Row {
			std::vector<Section> sections;
			ImU32 color = 0;
		};

		std::vector<Row> rows;
		bool showMiniMap = false;

		// support functions
		void processLine(
			const Line& line,
			const Config& config,
			size_t index,
			size_t column,
			size_t endColumn);
	} miniMap;

	// list of text markers
	struct Marker {
		Marker(ImU32 lineNumberColor, ImU32 textColor, const std::string_view& lineNumberTooltip, const std::string_view& textTooltip) :
			lineNumberColor(lineNumberColor), textColor(textColor), lineNumberTooltip(lineNumberTooltip), textTooltip(textTooltip) {}

		ImU32 lineNumberColor;
		ImU32 textColor;
		std::string lineNumberTooltip;
		std::string textTooltip;
	};

	using Markers = std::vector<Marker>;
	Markers markers;

	// list of squiggles
	struct Squiggle {
		Squiggle(size_t type, ImU32 color, const std::string_view& tooltip) : type(type), color(color), tooltip(tooltip) {}
		size_t type;
		ImU32 color;
		std::string tooltip;
	};

	using Squiggles = std::vector<Squiggle>;
	Squiggles squiggles;

	// autocomplete support
	class AutoComplete {
	public:
		// set the autocomplete configuration
		void setConfig(const AutoCompleteConfig* c);

		// request autocomplete mode based on triggers (and if allowed by current state)
		// return value indicates whether autocomplete was initiated
		bool startTyping(Cursors& cursors);
		bool startShortcut(Cursors& cursors);

		// cancel autocomplete mode (if required)
		void cancel();

		// update autocomplete state and render (if required)
		bool render(Document& document, Cursors& cursors, TypeSetter& typesetter, const Language* language, float textOffset, ImVec2 glyphSize);

		// specify a new set of suggestions
		void setSuggestions(const std::vector<std::string>& suggestions);

		// get information
		inline bool isActive() const { return active; }
		inline bool hasSuggestions() const { return state.suggestions.size() > 0 || state.suggestionsPromise; }
		static bool isSpecialKeyPressed();
		inline ImGuiKeyChord getTriggerShortcut() const { return configuration.triggerShortcut; }
		inline DocPos getStart() const { return startLocation; }
		inline std::string getReplacement() { return currentSelection < state.suggestions.size() ? state.suggestions[currentSelection] : ""; }

	private:
		// properties
		bool configured = false;
		bool active = false;
		bool requestActivation = false;
		bool requestDeactivation = false;
		DocPos currentLocation;
		DocPos startLocation;
		std::chrono::system_clock::time_point activationTime;
		AutoCompleteConfig configuration;
		AutoCompleteState state;
		bool triggeredManually = false;
		size_t currentSelection = 0;

		// support functions
		void start(Cursors& cursors);
		void updateState(Document& document, const Language* language);
		void refreshSuggestions();
	} autocomplete;

	// access the editor's text
	void setText(const std::string_view& text);

	// render (parts of) the text editor
	bool render(const char* title, const ImVec2& size, ImGuiChildFlags childFlags, ImGuiWindowFlags windowFlags);
	void renderActiveBracketBackground();
	void renderSelections();
	void renderTextMarkers();
	void renderMatchingBracketLines();
	void renderSquiggles();
	void renderText();
	void renderCursorCarets();
	void renderLineNumberMarkers();
	void renderLineNumbers();
	void renderDecorations();
	void renderFoldIndicators();
	void renderMiniMap();
	void renderScrollbarMiniMap();
	void renderPanScrollIndicator();
	void renderFindReplace();
	void renderPopups();

	// update editor state after changes caused by API calls or user interactions
	bool updateState();

	// keyboard and mouse interactions
	void handleKeyboardInputs();
	void handleMouseInteractions();

	// check visibility of a document position
	bool isDocPosVisible(DocPos pos) const;

	// coordinate transformation/normalization
	inline VisPos docPos2VisPos(DocPos pos) const { return typeSetter.docPos2VisPos(document, pos); }
	inline DocPos visPos2DocPos(VisPos pos) const { return typeSetter.visPos2DocPos(document, pos); }

	inline size_t normalizeLine(size_t line) const { return document.normalizeLine(line); }
	inline DocPos normalizePos(DocPos pos) const { return document.normalizePos(pos); }
	inline VisPos normalizePos(VisPos pos) const { return typeSetter.normalizePos(pos); }

	// manipulate selections/cursors
	void selectAll();
	void selectLine(size_t line);
	void selectLines(size_t startLine, size_t endLine);
	void selectRegion(DocPos start, DocPos end);
	void selectToBrackets(bool includeBrackets);
	void growSelections();
	void shrinkSelections();

	// clipboard actions
	void cut();
	void copy() const;
	void paste();
	void undo();
	void redo();

	// access cursor locations
	DocPos getCursorPosition(size_t cursor) const;
	DocSelection getCursorSelection(size_t cursor) const;

	// get information at mouse position (e.g. from ImGui::GetMousePos())
	bool isMousePosOverGlyph(const ImVec2& mousePos) const;
	DocPos getDocPosAtMousePos(const ImVec2& mousePos) const;
	std::string getWordAtMousePos(const ImVec2& mousePos) const;

	// scrolling support
	void setCursor(DocPos pos);
	void scrollToLine(size_t line, Scroll alignment, float fraction=0.0f);
	void handlePossibleScrolling();
	void makeCursorVisible();
	void resetScrolling();

	// find/replace support
	void selectFirstOccurrenceOf(const std::string_view& text, bool caseSensitive, bool wholeWord);
	void selectNextOccurrenceOf(const std::string_view& text, bool caseSensitive, bool wholeWord);
	void selectAllOccurrencesOf(const std::string_view& text, bool caseSensitive, bool wholeWord);
	void addNextOccurrence();
	void selectAllOccurrences();

	void replaceTextInCurrentCursor(const std::string_view& text);
	void replaceTextInAllCursors(const std::string_view& text);
	void replaceSectionText(const DocPos& start, const DocPos& end, const std::string_view& text);

	void openFindReplace();
	void closeFindReplace();
	void find();
	void findNext();
	void findAll();
	void replace();
	void replaceAll();

	// marker support
	void addMarker(size_t line, ImU32 lineNumberColor, ImU32 textColor, const std::string_view& lineNumberTooltip, const std::string_view& textTooltip);
	void clearMarkers();
	void compressMarkers();

	// squiggle support
	void addSquiggle(DocPos start, DocPos end, size_t type, ImU32 color, const std::string_view& tooltip);
	void clearSquiggles(size_t type);
	void clearSquiggles(DocPos start, DocPos end);
	void clearSquiggles();
	void compressSquiggles();

	// cursor/selection functions
	void moveUp(size_t rows, bool select);
	void moveDown(size_t rows, bool select);
	void moveLeft(bool select, bool wordMode);
	void moveRight(bool select, bool wordMode);
	inline void moveToTop(bool select) { moveTo(document.getTop(), select); }
	inline void moveToBottom(bool select) { moveTo(document.getBottom(), select); }
	inline void moveToStartOfLine(bool select) { moveTo(document.getStartOfLine(cursors.getCurrent().getInteractiveEnd()), select); }
	inline void moveToEndOfLine(bool select) { moveTo(document.getEndOfLine(cursors.getCurrent().getInteractiveEnd()), select); }
	void moveTo(DocPos coordinate, bool select);

	// add/delete characters
	void handleCharacter(ImWchar character);
	void handleBackspace(bool wordMode);
	void handleDelete(bool wordMode);

	// add/delete lines
	void removeSelectedLines();
	void insertLineAbove();
	void insertLineBelow();

	// transform selected lines
	void indentLines();
	void deindentLines();
	void moveUpLines();
	void moveDownLines();
	void toggleComments();

	// transform selections (filter function should accept and return UTF-8 encoded strings)
	void filterSelections(std::function<std::string(std::string_view)> filter);
	void selectionToLowerCase();
	void selectionToUpperCase();

	// transform entire document (filter function should accept and return UTF-8 encoded strings)
	void stripTrailingWhitespaces();
	void filterLines(std::function<std::string(std::string_view)> filter);
	void tabsToSpaces();
	void spacesToTabs();

	// transaction functions
	// note that strings must be UTF-8 encoded
	std::shared_ptr<Transaction> startTransaction(bool cancelsAutoComplete=true);
	bool endTransaction(std::shared_ptr<Transaction> transaction);

	void insertTextIntoAllCursors(std::shared_ptr<Transaction> transaction, const std::string_view& text);
	void deleteTextFromAllCursors(std::shared_ptr<Transaction> transaction);
	void autoIndentAllCursors(std::shared_ptr<Transaction> transaction);
	DocPos insertText(std::shared_ptr<Transaction> transaction, DocPos start, const std::string_view& text);
	void deleteText(std::shared_ptr<Transaction> transaction, DocPos start, DocPos end);

	// folding support
	inline bool isLineFoldable(size_t line) const { return lineFold.isFoldable(document, line); }
	inline bool isLineFolded(size_t line) const { return lineFold.isFolded(document, line); }
	inline bool isLineVisible(size_t line) const { return lineFold.isVisible(document, line); }
	inline bool isLineHidden(size_t line) const { return lineFold.isHidden(document, line); }

	inline void resetCursorAnimationTimer() { cursorAnimationTimer = -0.3f; }

	// rendering context
	static constexpr size_t invalidLine = std::numeric_limits<size_t>::max();

	bool firstFrame = true;
	float cursorWidth;
	ImVec2 cursorScreenPos;
	ImVec2 visibleSize;

	ImFont* font;
	float fontSize;
	float fontScaleDpi;
	ImVec2 glyphSize;

	float lineNumberLeftOffset;
	float lineNumberRightOffset;
	float decorationOffset;
	float foldIndicatorOffset;
	float textLeftOffset;
	float textRightOffset;
	float miniMapOffset;

	ImVec2 textSize;
	ImVec2 totalSize;
	size_t firstVisibleRow = 0;
	size_t lastVisibleRow = 0;
	size_t firstVisibleColumn = 0;
	size_t lastVisibleColumn = 0;

	float miniMapWidth;
	size_t firstMiniMapRow;
	size_t lastMiniMapRow;
	bool miniMapIsScrollbar = false;
	float miniMapScrollStart;
	float miniMapScrollY;

	float miniMapRowHeight;
	float miniMapColumnHeight;
	float miniMapColumnWidth;
	static constexpr float miniMapAlpha = 0.45f;
	static constexpr float miniMapViewPortAlpha = 0.15f;
	static constexpr float miniMapViewPortActiveAlpha = 0.3f;

	float cursorAnimationTimer = -0.3f;
	size_t scrollToLineNumber = invalidLine;
	Scroll scrollToAlignment = Scroll::alignMiddle;
	float scrollToFraction = 0.0f;
	DocPos ensureVisiblePos{invalidLine, 0};
	bool resetDearImGuiScrolling = false;

	size_t decoratorWidth = 0;
	std::function<void(Decorator&)> decoratorCallback;

	std::function<void(const CustomCaret&)> customCaretCallback;

	std::function<void(PopupData& data)> lineNumberContextMenuCallback;
	std::function<void(PopupData& data)> textContextMenuCallback;
	std::function<void(PopupData& data)> textHoverCallback;
	ImVec2 popUpWindowPos;
	DocPos popupDocPos;

	// find and replace context
	std::string findButtonLabel = "Find";
	std::string findAllButtonLabel = "Find All";
	std::string replaceButtonLabel = "Replace";
	std::string replaceAllButtonLabel = "Replace All";
	bool findReplaceVisible = false;
	bool focusOnEditor = false;
	bool focusOnFind = false;
	bool findCancelledAutocomplete = false;
	std::string findText;
	std::string replaceText;
	bool caseSensitiveFind = false;
	bool wholeWordFind = false;

	// interaction context
	bool navigatingVertically = false;
	float lastClickTime = -1.0f;
	ImWchar completePairCloser = 0;
	DocPos completePairLocation;
	bool panning = false;
	bool scrolling = false;
	ImVec2 scrollStart;
	bool selectingText = false;
	bool deletesHappened = false;
	std::function<void()> delayedChangeCallback;
	std::chrono::milliseconds delayedChangeDelay;
	std::chrono::system_clock::time_point delayedChangeReportTime;
	bool delayedChangeDetected = false;
	std::function<void()> languageChangeCallback;

	// color palette support
	void updatePalettes();
	static Palette defaultPalette;
	Palette paletteBase;
	Palette palette;
	Palette miniMapPalette;
	float paletteAlpha;
};
