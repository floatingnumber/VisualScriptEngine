#include "NUIE_NodeEditor.hpp"
#include "WAS_BitmapContextGdi.hpp"
#include "WAS_WindowsAppUtils.hpp"
#include "WAS_HwndEventHandler.hpp"
#include "WAS_ClipboardHandler.hpp"
#include "WAS_NodeEditorNodeTreeHwndControl.hpp"
#include "WAS_ParameterDialog.hpp"
#include "WAS_FileMenu.hpp"
#include "WAS_Toolbar.hpp"
#include "WAS_GdiplusUtils.hpp"

#include "BI_BuiltInNodes.hpp"
#include "NUIE_Localization.hpp"

#include "WAS_Direct2DContext.hpp"
#include "WAS_Direct2DOffscreenContext.hpp"

#include <windows.h>
#include <windowsx.h>
#include "resources.hpp"

#pragma comment (lib, "gdiplus.lib")
#pragma comment (lib, "comctl32.lib")
#pragma comment (lib, "windowscodecs.lib")
#pragma comment (lib, "d2d1.lib")
#pragma comment (lib, "dwrite.lib")

#define FILE_NEW		1101
#define FILE_OPEN		1102
#define FILE_SAVE		1103
#define FILE_QUIT		1104
#define EDIT_UNDO		1201
#define EDIT_REDO		1202
#define EDIT_SETTINGS	1203
#define EDIT_COPY		1204
#define EDIT_PASTE		1205
#define EDIT_DELETE		1206
#define EDIT_GROUP		1207
#define EDIT_UNGROUP	1208

static const WAS::NodeEditorNodeTreeHwndControl::Settings NodeTreeControlSettings;

static bool MessageBoxYesNo (HWND hwnd, LPCWSTR text, LPCWSTR caption)
{
	int result = MessageBox (hwnd, text, caption, MB_ICONWARNING | MB_YESNO);
	return (result == IDYES);
}

static const NUIE::BasicSkinParams& GetAppSkinParams ()
{
	static const NUIE::BasicSkinParams skinParams (
		/*backgroundColor*/ NUIE::Color (255, 255, 255),
		/*connectionLinePen*/ NUIE::Pen (NUIE::Color (38, 50, 56), 1.0),
		/*connectionMarker */ NUIE::SkinParams::ConnectionMarker::Circle,
		/*connectionMarkerSize*/ NUIE::Size (8.0, 8.0),
		/*nodePadding*/ 5.0,
		/*nodeBorderPen*/ NUIE::Pen (NUIE::Color (38, 50, 56), 1.0),
		/*nodeHeaderTextFont*/ NUIE::Font (L"Arial", 16.0),
		/*nodeHeaderTextColor*/ NUIE::Color (250, 250, 250),
		/*nodeHeaderErrorTextColor*/ NUIE::Color (250, 250, 250),
		/*nodeHeaderBackgroundColor*/ NUIE::Color (41, 127, 255),
		/*nodeHeaderErrorBackgroundColor*/ NUIE::Color (199, 80, 80),
		/*nodeContentTextFont*/ NUIE::Font (L"Arial", 14.0),
		/*nodeContentTextColor*/ NUIE::Color (0, 0, 0),
		/*nodeContentBackgroundColor*/ NUIE::Color (236, 236, 236),
		/*slotTextColor*/ NUIE::Color (0, 0, 0),
		/*slotTextBackgroundColor*/ NUIE::Color (246, 246, 246),
		/*slotMarker*/ NUIE::SkinParams::SlotMarker::Circle,
		/*slotMarkerSize*/ NUIE::Size (8.0, 8.0),
		/*selectionBlendColor*/ NUIE::BlendColor (NUIE::Color (41, 127, 255), 0.25),
		/*disabledBlendColor*/ NUIE::BlendColor (NUIE::Color (0, 138, 184), 0.2),
		/*selectionRectPen*/ NUIE::Pen (NUIE::Color (41, 127, 255), 1.0),
		/*nodeSelectionRectPen*/ NUIE::Pen (NUIE::Color (41, 127, 255), 3.0),
		/*buttonBorderPen*/ NUIE::Pen (NUIE::Color (146, 152, 155), 1.0),
		/*buttonBackgroundColor*/ NUIE::Color (217, 217, 217),
		/*textPanelTextColor*/ NUIE::Color (0, 0, 0),
		/*textPanelBackgroundColor*/ NUIE::Color (236, 236, 236),
		/*groupNameFont*/ NUIE::Font (L"Arial", 16.0),
		/*groupNameColor*/ NUIE::Color (0, 0, 0),
		/*groupBackgroundColors*/ NUIE::NamedColorSet ({
			{ NE::LocalizeString (L"Blue"), NUIE::Color (160, 200, 240) },
			{ NE::LocalizeString (L"Green"), NUIE::Color (160, 239, 160) },
			{ NE::LocalizeString (L"Red"), NUIE::Color (239, 189, 160) }
		}),
		/*groupPadding*/ 10.0
	);
	return skinParams;
}

static void AddNodeTreeItem (NUIE::NodeTree& nodeTree, size_t groupIndex, const std::wstring& name, const NUIE::IconId& iconId, const NUIE::CreatorFunction& creator)
{
	NUIE::IconId nodeIconId (iconId.GetId () + TREE_TO_NODE_ICON_OFFSET);
	nodeTree.AddItem (groupIndex, name, iconId, [=] (const NUIE::Point& position) {
		NUIE::UINodePtr node = creator (position);
		BI::BasicUINodePtr basicNode = std::dynamic_pointer_cast<BI::BasicUINode> (node);
		if (basicNode != nullptr) {
			basicNode->SetIconId (nodeIconId);
		}
		return node;
	});
}

class MyResourceImageLoader : public WAS::Direct2DImageLoaderFromResource
{
	virtual HRSRC GetImageResHandle (const NUIE::IconId& iconId) override
	{
		HRSRC resHandle = FindResource (NULL, MAKEINTRESOURCE (iconId.GetId ()), L"IMAGE");
		return resHandle;
	}
};

class AppUIEnvironment : public NUIE::NodeUIEnvironment
{
public:
	AppUIEnvironment () :
		NUIE::NodeUIEnvironment (),
		stringConverter (NE::BasicStringConverter (WAS::GetStringSettingsFromSystem ())),
		skinParams (GetAppSkinParams ()),
		eventHandler (),
		clipboardHandler (),
		evaluationEnv (nullptr),
		nodeEditorControl (NodeTreeControlSettings, NUIE::NativeDrawingContextPtr (new WAS::Direct2DContext (WAS::Direct2DImageLoaderPtr (new MyResourceImageLoader ())))),
		fileMenu (nullptr),
		toolbar (nullptr)
	{
	
	}

	void Init (NUIE::NodeEditor* nodeEditorPtr, WAS::FileMenu* fileMenuPtr, WAS::Toolbar* toolbarPtr, HWND parentHandle)
	{
		class ImageLoader : public WAS::NodeEditorNodeTreeHwndControl::ImageLoader
		{
		public:
			virtual HBITMAP LoadGroupClosedImage (COLORREF bgColor) override
			{
				return WAS::LoadBitmapFromResource (MAKEINTRESOURCE (FOLDERCLOSED_ICON), L"IMAGE", bgColor);
			}

			virtual HBITMAP LoadGroupOpenedImage (COLORREF bgColor) override
			{
				return WAS::LoadBitmapFromResource (MAKEINTRESOURCE (FOLDEROPENED_ICON), L"IMAGE", bgColor);
			}

			virtual HBITMAP LoadImage (const NUIE::IconId& iconId, COLORREF bgColor) override
			{
				return WAS::LoadBitmapFromResource (MAKEINTRESOURCE (iconId.GetId ()), L"IMAGE", bgColor);
			}
		};

		NUIE::NodeTree nodeTree;

		size_t inputNodes = nodeTree.AddGroup (L"Input Nodes");
		AddNodeTreeItem (nodeTree, inputNodes, L"Boolean", NUIE::IconId (TREE_BOOLEAN_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::BooleanNode (NE::LocString (L"Boolean"), position, true));
		});
		AddNodeTreeItem (nodeTree, inputNodes, L"Integer", NUIE::IconId (TREE_INTEGER_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::IntegerUpDownNode (NE::LocString (L"Integer"), position, 0, 1));
		});
		AddNodeTreeItem (nodeTree, inputNodes, L"Number", NUIE::IconId (TREE_DOUBLE_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::DoubleUpDownNode (NE::LocString (L"Number"), position, 0.0, 1.0));
		});
		AddNodeTreeItem (nodeTree, inputNodes, L"Integer Increment", NUIE::IconId (TREE_INTEGERINCREMENTED_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::IntegerIncrementedNode (NE::LocString (L"Integer Increment"), position));
		});
		AddNodeTreeItem (nodeTree, inputNodes, L"Number Increment", NUIE::IconId (TREE_DOUBLEINCREMENTED_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::DoubleIncrementedNode (NE::LocString (L"Number Increment"), position));
		});
		AddNodeTreeItem (nodeTree, inputNodes, L"Number Distribution", NUIE::IconId (TREE_DOUBLEDISTRIBUTED_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::DoubleDistributedNode (NE::LocString (L"Number Distribution"), position));
		});
		size_t mathematicalNodes = nodeTree.AddGroup (L"Mathematical Nodes");
		AddNodeTreeItem (nodeTree, mathematicalNodes, L"Addition", NUIE::IconId (TREE_ADDITION_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::AdditionNode (NE::LocString (L"Addition"), position));
		});
		AddNodeTreeItem (nodeTree, mathematicalNodes, L"Subtraction", NUIE::IconId (TREE_SUBTRACTION_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::SubtractionNode (NE::LocString (L"Subtraction"), position));
		});
		AddNodeTreeItem (nodeTree, mathematicalNodes, L"Multiplication", NUIE::IconId (TREE_MULTIPLICATION_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::MultiplicationNode (NE::LocString (L"Multiplication"), position));
		});
		AddNodeTreeItem (nodeTree, mathematicalNodes, L"Division", NUIE::IconId (TREE_DIVISION_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::DivisionNode (NE::LocString (L"Division"), position));
		});
		AddNodeTreeItem (nodeTree, mathematicalNodes, L"Floor", NUIE::IconId (TREE_FLOOR_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::FloorNode (NE::LocString (L"Floor"), position));
		});
		AddNodeTreeItem (nodeTree, mathematicalNodes, L"Ceil", NUIE::IconId (TREE_CEIL_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::CeilNode (NE::LocString (L"Ceil"), position));
		});
		AddNodeTreeItem (nodeTree, mathematicalNodes, L"Abs", NUIE::IconId (TREE_ABS_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::AbsNode (NE::LocString (L"Abs"), position));
		});
		AddNodeTreeItem (nodeTree, mathematicalNodes, L"Negative", NUIE::IconId (TREE_NEGATIVE_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::NegativeNode (NE::LocString (L"Negative"), position));
		});
		AddNodeTreeItem (nodeTree, mathematicalNodes, L"Sqrt", NUIE::IconId (TREE_SQRT_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::SqrtNode (NE::LocString (L"Sqrt"), position));
		});
		size_t otherNodes = nodeTree.AddGroup (L"Other Nodes");
		AddNodeTreeItem (nodeTree, otherNodes, L"List Builder", NUIE::IconId (TREE_LISTBUILDER_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::ListBuilderNode (NE::LocString (L"List Builder"), position));
		});
		AddNodeTreeItem (nodeTree, otherNodes, L"Viewer", NUIE::IconId (TREE_VIEWER_ICON), [&] (const NUIE::Point& position) {
			return NUIE::UINodePtr (new BI::MultiLineViewerNode (NE::LocString (L"Viewer"), position, 5));
		});

		RECT clientRect;
		GetClientRect (parentHandle, &clientRect);
		int width = clientRect.right - clientRect.left;
		int height = clientRect.bottom - clientRect.top;

		ImageLoader imageLoader;
		nodeEditorControl.Init (nodeEditorPtr, parentHandle, 0, 0, width, height);
		nodeEditorControl.FillNodeTree (nodeTree, &imageLoader);
		eventHandler.Init ((HWND) nodeEditorControl.GetEditorNativeHandle ());

		fileMenu = fileMenuPtr;
		toolbar = toolbarPtr;

		EnableCommand (EDIT_SETTINGS, false);
		EnableCommand (EDIT_COPY, false);
		EnableCommand (EDIT_DELETE, false);
		EnableCommand (EDIT_GROUP, false);
		EnableCommand (EDIT_UNGROUP, false);
		EnableCommand (EDIT_UNDO, false);
		EnableCommand (EDIT_REDO, false);
		EnableCommand (EDIT_PASTE, clipboardHandler.HasClipboardContent ());
	}

	void OnResize (int x, int y, int width, int height)
	{
		int toolbarHeight = 0;
		if (toolbar != nullptr) {
			toolbarHeight = toolbar->GetHeight ();
		}
		nodeEditorControl.Resize (x, y + toolbarHeight, width, height - toolbarHeight);
	}

	void OnClipboardChanged ()
	{
		EnableCommand (EDIT_PASTE, clipboardHandler.HasClipboardContent ());
	}

	virtual const NE::StringConverter& GetStringConverter () override
	{
		return stringConverter;
	}

	virtual const NUIE::SkinParams& GetSkinParams () override
	{
		return skinParams;
	}

	virtual NUIE::DrawingContext& GetDrawingContext () override
	{
		return nodeEditorControl.GetDrawingContext ();
	}

	virtual double GetWindowScale () override
	{
		return 1.0;
	}

	virtual NE::EvaluationEnv& GetEvaluationEnv () override
	{
		return evaluationEnv;
	}

	virtual void OnEvaluationBegin () override
	{

	}

	virtual void OnEvaluationEnd () override
	{

	}

	virtual void OnValuesRecalculated () override
	{
		
	}

	virtual void OnRedrawRequested () override
	{
		nodeEditorControl.Invalidate ();
	}

	virtual NUIE::EventHandler& GetEventHandler () override
	{
		return eventHandler;
	}

	virtual NUIE::ClipboardHandler& GetClipboardHandler () override
	{
		return clipboardHandler;
	}

	virtual void OnSelectionChanged (const NUIE::Selection& selection) override
	{
		bool hasSelection = !selection.IsEmpty ();
		EnableCommand (EDIT_SETTINGS, hasSelection);
		EnableCommand (EDIT_COPY, hasSelection);
		EnableCommand (EDIT_DELETE, hasSelection);
		EnableCommand (EDIT_GROUP, hasSelection);
		EnableCommand (EDIT_UNGROUP, hasSelection);
	}

	virtual void OnUndoStateChanged (const NUIE::UndoState& undoState) override
	{
		EnableCommand (EDIT_UNDO, undoState.CanUndo ());
		EnableCommand (EDIT_REDO, undoState.CanRedo ());
	}

	virtual void OnClipboardStateChanged (const NUIE::ClipboardState& clipboardState) override
	{
		EnableCommand (EDIT_PASTE, clipboardState.HasContent ());
	}

	virtual void OnIncompatibleVersionPasted (const NUIE::Version&) override
	{

	}

private:
	void EnableCommand (UINT commandId, bool isEnabled)
	{
		fileMenu->EnableItem (commandId, isEnabled);
		toolbar->EnableItem (commandId, isEnabled);
	}

	NE::BasicStringConverter			stringConverter;
	NUIE::BasicSkinParams				skinParams;
	WAS::HwndEventHandler				eventHandler;
	WAS::ClipboardHandler				clipboardHandler;
	NE::EvaluationEnv					evaluationEnv;
	WAS::NodeEditorNodeTreeHwndControl	nodeEditorControl;
	WAS::FileMenu*						fileMenu;
	WAS::Toolbar*						toolbar;
};

class Application
{
public:
	Application () :
		uiEnvironment (),
		nodeEditor (uiEnvironment),
		fileMenu (),
		toolbar (),
		fileFilter ({ L"Visual Script Engine", L"vse" })
	{

	}

	void Init (HWND hwnd)
	{
		InitFileMenu (hwnd);
		InitToolbar (hwnd);
		uiEnvironment.Init (&nodeEditor, &fileMenu, &toolbar, hwnd);
	}

	void New (HWND hwnd)
	{
		if (nodeEditor.NeedToSave ()) {
			bool result = MessageBoxYesNo (hwnd, L"You have made some changes that are not saved. Would you like to start new file?", L"New File");
			if (!result) {
				return;
			}
		}
		nodeEditor.New ();
	}

	void Open (HWND hwnd)
	{
		if (nodeEditor.NeedToSave ()) {
			bool result = MessageBoxYesNo (hwnd, L"You have made some changes that are not saved. Would you like to open file?", L"Open File");
			if (!result) {
				return;
			}
		}
		std::wstring fileName;
		if (WAS::OpenFileDialog (hwnd, fileFilter, fileName)) {
			nodeEditor.Open (fileName);
			nodeEditor.AlignToWindow ();
		}
	}

	void Save (HWND hwnd)
	{
		std::wstring fileName;
		if (WAS::SaveFileDialog (hwnd, fileFilter, fileName)) {
			nodeEditor.Save (fileName);
		}
	}

	bool Close (HWND hwnd)
	{
		if (nodeEditor.NeedToSave ()) {
			return MessageBoxYesNo (hwnd, L"You have made some changes that are not saved. Would you like to quit?", L"Quit");
		}
		return true;
	}

	void ExecuteCommand (NUIE::CommandCode command)
	{
		nodeEditor.ExecuteCommand (command);
	}

	void OnResize (int x, int y, int width, int height)
	{
		uiEnvironment.OnResize (x, y, width, height);
	}

	void OnClipboardChanged ()
	{
		uiEnvironment.OnClipboardChanged ();
	}

private:
	void InitFileMenu (HWND hwnd)
	{
		HMENU file = fileMenu.AddPopupMenu (L"File");
		fileMenu.AddPopupMenuItem (file, FILE_NEW, L"New");
		fileMenu.AddPopupMenuItem (file, FILE_OPEN, L"Open");
		fileMenu.AddPopupMenuItem (file, FILE_SAVE, L"Save");
		fileMenu.AddPopupMenuSeparator (file);
		fileMenu.AddPopupMenuItem (file, FILE_QUIT, L"Quit");

		HMENU edit = fileMenu.AddPopupMenu (L"Edit");
		fileMenu.AddPopupMenuItem (edit, EDIT_UNDO, L"Undo");
		fileMenu.AddPopupMenuItem (edit, EDIT_REDO, L"Redo");
		fileMenu.AddPopupMenuSeparator (edit);
		fileMenu.AddPopupMenuItem (edit, EDIT_SETTINGS, L"Settings");
		fileMenu.AddPopupMenuItem (edit, EDIT_COPY, L"Copy");
		fileMenu.AddPopupMenuItem (edit, EDIT_PASTE, L"Paste");
		fileMenu.AddPopupMenuItem (edit, EDIT_DELETE, L"Delete");
		fileMenu.AddPopupMenuSeparator (edit);
		fileMenu.AddPopupMenuItem (edit, EDIT_GROUP, L"Group");
		fileMenu.AddPopupMenuItem (edit, EDIT_UNGROUP, L"Ungroup");

		SetMenu (hwnd, fileMenu.GetMenuBar ());
	}

	void InitToolbar (HWND hwnd)
	{
		toolbar.Init (hwnd);

		AddToolbarItem (TOOLBAR_ENABLED_NEW_ICON, FILE_NEW);
		AddToolbarItem (TOOLBAR_ENABLED_OPEN_ICON, FILE_OPEN);
		AddToolbarItem (TOOLBAR_ENABLED_SAVE_ICON, FILE_SAVE);
		toolbar.AddSeparator ();
		
		AddToolbarItem (TOOLBAR_ENABLED_UNDO_ICON, EDIT_UNDO);
		AddToolbarItem (TOOLBAR_ENABLED_REDO_ICON, EDIT_REDO);
		toolbar.AddSeparator ();

		AddToolbarItem (TOOLBAR_ENABLED_SETTINGS_ICON, EDIT_SETTINGS);
		AddToolbarItem (TOOLBAR_ENABLED_COPY_ICON, EDIT_COPY);
		AddToolbarItem (TOOLBAR_ENABLED_PASTE_ICON, EDIT_PASTE);
		AddToolbarItem (TOOLBAR_ENABLED_DELETE_ICON, EDIT_DELETE);
		toolbar.AddSeparator ();

		AddToolbarItem (TOOLBAR_ENABLED_GROUP_ICON, EDIT_GROUP);
		AddToolbarItem (TOOLBAR_ENABLED_UNGROUP_ICON, EDIT_UNGROUP);
	}

	void AddToolbarItem (WORD imageResourceId, UINT commandId)
	{
		COLORREF bgColor = GetSysColor (COLOR_3DFACE);
		HBITMAP bitmap = WAS::LoadBitmapFromResource (MAKEINTRESOURCE (imageResourceId), L"IMAGE", bgColor);
		HBITMAP disabledBitmap = WAS::LoadBitmapFromResource (MAKEINTRESOURCE (imageResourceId + ENABLED_TO_DISABLED_ICON_OFFSET), L"IMAGE", bgColor);
		toolbar.AddItem (bitmap, disabledBitmap, commandId);
	}

	AppUIEnvironment	uiEnvironment;
	NUIE::NodeEditor	nodeEditor;
	WAS::FileMenu		fileMenu;
	WAS::Toolbar		toolbar;
	WAS::FileFilter		fileFilter;
};

LRESULT CALLBACK ApplicationWindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_CREATE) {
		LPCREATESTRUCT createStruct = LPCREATESTRUCT (lParam);
		SetWindowLongPtr (hwnd, GWLP_USERDATA, (LONG_PTR) createStruct->lpCreateParams);
	} else if (msg == WM_DESTROY) {
		SetWindowLongPtr (hwnd, GWLP_USERDATA, NULL);
		PostQuitMessage (0);
	}

	Application* application = (Application*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (application == nullptr) {
		return DefWindowProc (hwnd, msg, wParam, lParam);
	}

	switch (msg) {
		case WM_CREATE:
			{
				application->Init (hwnd);
			}
			break;
		case WM_CLOSE:
			{
				if (application->Close (hwnd)) {
					DestroyWindow (hwnd);
				}
				return 0;
			}
		case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO minMaxInfo = (LPMINMAXINFO) lParam;
				minMaxInfo->ptMinTrackSize.x = 600;
				minMaxInfo->ptMinTrackSize.y = 400;
			}
			break;
		case WM_SIZE:
			{
				int newWidth = LOWORD (lParam);
				int newHeight = HIWORD (lParam);
				
				application->OnResize (0, 0, newWidth, newHeight);
			}
			break;
		case WM_CLIPBOARDUPDATE:
			{
				application->OnClipboardChanged ();
			}
			break;
		case WM_COMMAND:
			{
				WORD command = LOWORD (wParam);
				switch (command) {
					case FILE_NEW:
						application->New (hwnd);
						break;
					case FILE_OPEN:
						application->Open (hwnd);
						break;
					case FILE_SAVE:
						application->Save (hwnd);
						break;
					case EDIT_UNDO:
						application->ExecuteCommand (NUIE::CommandCode::Undo);
						break;
					case EDIT_REDO:
						application->ExecuteCommand (NUIE::CommandCode::Redo);
						break;
					case EDIT_SETTINGS:
						application->ExecuteCommand (NUIE::CommandCode::SetParameters);
						break;
					case EDIT_COPY:
						application->ExecuteCommand (NUIE::CommandCode::Copy);
						break;
					case EDIT_PASTE:
						application->ExecuteCommand (NUIE::CommandCode::Paste);
						break;
					case EDIT_DELETE:
						application->ExecuteCommand (NUIE::CommandCode::Delete);
						break;
					case EDIT_GROUP:
						application->ExecuteCommand (NUIE::CommandCode::Group);
						break;
					case EDIT_UNGROUP:
						application->ExecuteCommand (NUIE::CommandCode::Ungroup);
						break;
					case FILE_QUIT:
						SendMessage (hwnd, WM_CLOSE, 0, 0);
						break;
				}
			}
			break;
	}

	return DefWindowProc (hwnd, msg, wParam, lParam);
}

int wWinMain (HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	Application application;

	WNDCLASSEX windowClass;
	ZeroMemory (&windowClass, sizeof (WNDCLASSEX));
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = 0;
	windowClass.lpfnWndProc = ApplicationWindowProc;
	windowClass.style = CS_DBLCLKS;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInstance;
	windowClass.hIcon = LoadIcon (hInstance, MAKEINTRESOURCE (APPLICATION_ICON));
	windowClass.hIconSm = LoadIcon (hInstance, MAKEINTRESOURCE (APPLICATION_ICON));
	windowClass.hCursor = LoadCursor (NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH) COLOR_WINDOW;
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = L"VisualScriptEngineDemo";

	if (!RegisterClassEx (&windowClass)) {
		return false;
	}

	RECT requiredRect = { 0, 0, 1200, 800 };
	AdjustWindowRect (&requiredRect, WS_OVERLAPPEDWINDOW, false);

	HWND windowHandle = CreateWindowEx (
		WS_EX_WINDOWEDGE, windowClass.lpszClassName, L"Visual Script Engine Demo", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, requiredRect.right - requiredRect.left, requiredRect.bottom - requiredRect.top, NULL, NULL, NULL, &application
	);

	if (windowHandle == NULL) {
		return 1;
	}

	AddClipboardFormatListener (windowHandle);
	ShowWindow (windowHandle, SW_SHOW);
	UpdateWindow (windowHandle);

	MSG msg;
	while (GetMessage (&msg, NULL, 0, 0)) {
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	RemoveClipboardFormatListener (windowHandle);
	return 0;
}