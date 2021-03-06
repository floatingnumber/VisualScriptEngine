#ifndef WAS_NODEEDITORNODETREEHWNDCONTROL_HPP
#define WAS_NODEEDITORNODETREEHWNDCONTROL_HPP

#include "NUIE_NodeEditor.hpp"
#include "NUIE_NodeTree.hpp"
#include "WAS_NodeEditorHwndControl.hpp"
#include "WAS_NodeTree.hpp"

namespace WAS
{

class NodeEditorNodeTreeHwndControl : public NUIE::NativeNodeEditorControl
{
public:
	class Settings
	{
	public:
		Settings (int treeWidth, int treeRightMargin);

		int		treeWidth;
		int		treeRightMargin;
	};

	class ImageLoader
	{
	public:
		ImageLoader ();
		virtual ~ImageLoader ();

		virtual HBITMAP LoadGroupClosedImage () = 0;
		virtual HBITMAP LoadGroupOpenedImage () = 0;
		virtual HBITMAP LoadImage (const NUIE::IconId& iconId) = 0;
	};

	NodeEditorNodeTreeHwndControl (const Settings& settings);
	NodeEditorNodeTreeHwndControl (const Settings& settings, const NUIE::NativeDrawingContextPtr& nativeContext);
	virtual ~NodeEditorNodeTreeHwndControl ();

	virtual void					EnableInputHandling (bool enabled) override;

	virtual bool					Init (NUIE::NodeEditor* nodeEditorPtr, void* nativeParentHandle, int x, int y, int width, int height) override;
	virtual void*					GetEditorNativeHandle () const override;
	virtual bool					IsMouseOverEditorWindow () const override;

	virtual void					Resize (int x, int y, int width, int height) override;
	virtual void					Invalidate () override;
	virtual void					Draw () override;
	
	virtual NUIE::DrawingContext&	GetDrawingContext () override;

	void							FillNodeTree (const NUIE::NodeTree& nodeTree, ImageLoader* imageLoader);
	void							TreeViewDoubleClick (LPNMHDR lpnmhdr);
	void							TreeViewSelectionChanged (LPNMTREEVIEW lpnmtv);
	void							TreeViewItemExpanded (LPNMTREEVIEW lpnmtv);
	void							TreeViewBeginDrag (LPNMTREEVIEW lpnmtv);
	void							TreeViewEndDrag (int x, int y);

private:
	void							CreateNode (LPARAM nodeId, int screenX, int screenY);

	Settings											settings;
	NodeTreeView										nodeTreeView;
	NodeEditorHwndControl								nodeEditorControl;
	CustomControl										mainControl;

	LPARAM												selectedNode;
	LPARAM												draggedNode;
	std::unordered_map<LPARAM, NUIE::CreatorFunction>	nodeIdToCreator;
};

}

#endif
