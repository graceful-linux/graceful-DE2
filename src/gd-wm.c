//
// Created by dingjing on 25-6-27.
//

#include "gd-wm.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>

struct _GDWm
{
    GObject         parent;

    Atom            wmCheckAtom;
    Atom            wmNameAtom;
    Atom            utf8StringAtom;
    Window          wmCheck;
};
G_DEFINE_TYPE (GDWm, gd_wm, G_TYPE_OBJECT)


static char * get_wm_name (GDWm *self)
{
    Atom actualType;
    int actualFormat;
    unsigned long nItems;
    unsigned long bytesAfter;
    unsigned char* prop = NULL;

    GdkDisplay* display = gdk_display_get_default ();
    Display* xdisplay = gdk_x11_display_get_xdisplay (display);

    gdk_x11_display_error_trap_push (display);

    const int result = XGetWindowProperty (xdisplay,
                               self->wmCheck,
                               self->wmNameAtom,
                               0,
                               G_MAXLONG,
                               False,
                               self->utf8StringAtom,
                               &actualType,
                               &actualFormat,
                               &nItems,
                               &bytesAfter,
                               &prop);

    gdk_x11_display_error_trap_pop_ignored (display);

    if (result != Success || actualType != self->utf8StringAtom || actualFormat != 8 || nItems == 0) {
        XFree (prop);
        return NULL;
    }

    char *wmName = g_strndup ((const char *) prop, nItems);
    XFree (prop);

    if (!g_utf8_validate (wmName, -1, NULL)) {
        g_free (wmName);
        return NULL;
    }

    return wmName;
}


static void update_wm_keybindings (GDWm *self)
{
    GdkDisplay *display = gdk_display_get_default ();
    Display *xdisplay = gdk_x11_display_get_xdisplay (display);

    char* wmName = get_wm_name (self);
    char* wmKeybindings = g_strdup_printf ("%s,graceful-DE2", wmName ? wmName : "Unknown");

    gdk_x11_display_error_trap_push (display);

    /**
     * _GNOME_WM_KEYBINGDINGS是一个较老的、非标准但在GNOME2.x/Metacity使用的X window窗口属性
     * 让客户端窗口声明某些键盘快捷键希望由窗口管理器处理，而不是自己处理。
     *
     * 客户端通过这个设置告诉窗口管理器：
     *  当我拥有焦点时候，如果用户按下某些特定按键(如: F1、Ctrl+Alt+T)，请你拦截这个按键并执行你的功能，
     *  而不是让我处理
     *
     * 设计目的：
     *  1. 解决键盘快捷键冲突：比如你写一个窗口时候不希望屏蔽全局快捷键
     *  2. 让窗口管理器统一处理部分键绑定逻辑，即使某窗口全屏或抓住了焦点
     *
     * 这个值通常是一个UTF8_STRING，内容为一个空格分割的字符串列表：
     * _GNOME_WM_KEYBINDINGS(UTF8_STRING) = "panel:run_dialog" "help"
     *  - "panel:run_dialog"：表示这个窗口希望窗口管理器处理"打开运行对话框"的快捷键
     *  - "help": 让窗口管理器处理 F1 帮助键
     */
    XChangeProperty (xdisplay,
                   self->wmCheck,
                   XInternAtom (xdisplay, "_GRACEFUL_WM_KEYBINDINGS", False),
                   self->utf8StringAtom,
                   8,
                   PropModeReplace,
                   (guchar*) wmKeybindings,
                   strlen (wmKeybindings));

    gdk_x11_display_error_trap_pop_ignored (display);

    g_free (wmKeybindings);
    g_free (wmName);
}

static Window get_net_supporting_wm_check (GDWm* self, GdkDisplay *display, Window window)
{
    Atom actualType;
    int actualFormat;
    unsigned long nItems;
    unsigned long bytesAfter;
    unsigned char* prop = NULL;

    Display *xdisplay = gdk_x11_display_get_xdisplay (display);
    gdk_x11_display_error_trap_push (display);

    const int result = XGetWindowProperty (xdisplay,
                               window,
                               self->wmCheckAtom,
                               0,
                               G_MAXLONG,
                               False,
                               XA_WINDOW,
                               &actualType,
                               &actualFormat,
                               &nItems,
                               &bytesAfter,
                               &prop);

    gdk_x11_display_error_trap_pop_ignored (display);

    if (result != Success || actualType != XA_WINDOW || nItems == 0) {
        XFree (prop);
        return None;
    }

    const Window wmCheck = *(Window*) prop;
    XFree (prop);

    return wmCheck;
}

static void update_wm_check (GDWm *self)
{
    GdkDisplay *display = gdk_display_get_default ();
    Display *xdisplay = gdk_x11_display_get_xdisplay (display);
    const Window wmCheck = get_net_supporting_wm_check (self, display, XDefaultRootWindow (xdisplay));

    if (wmCheck == None) {
        return;
    }

    if (wmCheck != get_net_supporting_wm_check (self, display, wmCheck)) {
        return;
    }
    self->wmCheck = wmCheck;

    /**
     * GDK中用于检测x11操作是否发生错误的安全机制，与 gdk_x11_display_error_trap_pop 成对使用。
     * 二者之间包含可能出错的操作，执行完后使用 gdk_x11_display_error_trap_pop 返回值确定是否有错误发生
     */

    gdk_x11_display_error_trap_push (display);
    XSelectInput (xdisplay, self->wmCheck, StructureNotifyMask);

    if (gdk_x11_display_error_trap_pop (display) != 0) {
        self->wmCheck = None;
    }

    update_wm_keybindings (self);
}

static GdkFilterReturn event_filter_cb (GdkXEvent *xevent, GdkEvent* event, void* data)
{
    XEvent* e = xevent;
    GDWm* self = GD_WM (data);

    if (self->wmCheck != None) {
        if (e->type == DestroyNotify && e->xdestroywindow.window == self->wmCheck) {
            update_wm_check (self);
        }
        else if (e->type == PropertyNotify && e->xproperty.window == self->wmCheck) {
            if (e->xproperty.atom == self->wmCheckAtom) {
                update_wm_check (self);
            }
            else if (e->xproperty.atom == self->wmNameAtom) {
                update_wm_keybindings (self);
            }
        }
    }
    else {
        GdkDisplay *display = gdk_display_get_default ();
        Display *xdisplay = gdk_x11_display_get_xdisplay (display);
        if (e->type == PropertyNotify
            && e->xproperty.window == XDefaultRootWindow (xdisplay)
            && e->xproperty.atom == self->wmCheckAtom) {
            update_wm_check (self);
        }
    }

    return GDK_FILTER_CONTINUE;
}

static void gd_wm_finalize (GObject *object)
{
    GDWm *self = GD_WM (object);

    gdk_window_remove_filter (NULL, event_filter_cb, self);

    G_OBJECT_CLASS (gd_wm_parent_class)->finalize (object);
}

static void gd_wm_class_init (GDWmClass *self)
{
    GObjectClass *objClass = G_OBJECT_CLASS (self);
    objClass->finalize = gd_wm_finalize;
}

static void gd_wm_init (GDWm *self)
{
    GdkDisplay *display;
    Display *xdisplay;
    Window xroot;
    XWindowAttributes attrs;

    display = gdk_display_get_default ();
    xdisplay = gdk_x11_display_get_xdisplay (display);

    /**
     * 获取指定 Display 默认根窗口。
     * 在X window系统中，每个屏幕(screen)都有一个根窗口，它是该屏幕上所有窗口的父窗口，
     * 通常你看到的桌面背景其实就是这个根窗口
     * 常用于：
     *  1. 创建全屏窗口时作为父窗口
     *  2. 拦截全局鼠标/键盘事件
     *  3. 获取显示的屏幕分辨率
     *  4. 与窗口管理器通信(如: 发送事件、检测属性)
     * 如果获取多个屏幕的根窗口，不要使用 DefaultRootWindow，而是直接使用：RootWindow(display, screenNumber)
     */
    xroot = DefaultRootWindow (xdisplay);

    /**
     * _NET_SUPPORTING_WM_CHECK 是EWMH(Extended Window Manager Hints)标准中定义的一个窗口属性，它用于：
     *  1. 识别当前运行的窗口管理器，并让应用程序能确认一个符合EWHM规范的WM正在运行。
     *  2. 获取窗口管理器的窗口ID（用于进一步读取其他属性, 如: _NET_WM_NAME）
     *  3. 自检，防止某些恶意程序伪装成WM
     *
     * 工作机制：
     *  1. 窗口管理器启动时候，会在根窗口上设置 _NET_SUPPORTING_WM_CHECK = <一个特殊的窗口ID>
     *  2. 然后它在这个窗口ID上再设置同样的属性，即：
     *      <window_id>._NET_SUPPORTING_WM_CHECK = <同一个 window_id>
     *     这构成一个自指属性，表示这个窗口是负责管理窗口的窗口管理器本身。
     *
     * 为什么这么做：
     *  1. 应用程序或工具(如: xprop, wmctrl, xface-panel, kwin, chrome)想确认当前系统中：
     *      a. 是否有EWMH兼容的窗口管理器？
     *      b. 是哪个窗口管理器(通过: _NET_WM_NAME)
     *  2. 避免多个窗口管理器争抢 root window 的事件监听(比如: SubstructureRedirectMask)
     *
     * 总之一句话，_NET_SUPPORTING_WM_CHECK是当前窗口管理器留下的“签名”，用于告诉别的程序：我是EWMH兼容的窗口管理器，
     * 这是我的身份窗口。
     */
    self->wmCheckAtom = XInternAtom (xdisplay, "_NET_SUPPORTING_WM_CHECK", False);

    self->wmNameAtom = XInternAtom (xdisplay, "_NET_WM_NAME", False);
    self->utf8StringAtom = XInternAtom (xdisplay, "UTF8_STRING", False);

    /**
     * 对底层原生事件进行拦截和处理。
     * 在事件传递到GTK之前，先对其进行过滤、拦截、修改，甚至屏蔽处理
     * 第一个参数：NULL 表示对所有窗口的事件生效(全局事件过滤)
     * 第二个参数：GdkFilterReturn filter_func(GdkXEvent*, GdkEvent* event, void* data);
     *          其中第一个参数指向平台原生事件的指针(例如：x11上是XEvent*)
     *          其中第二个参数指向GDK的事件对象，可能是NULL
     *          其中第三个参数data：用户传入的数据
     */
    gdk_window_add_filter (NULL, event_filter_cb, self);

    XGetWindowAttributes (xdisplay, xroot, &attrs);

    /**
     * 当前程序希望监听 root 窗口发生的某些事件，PropertyChaneMask 表示属性改变事件
     * 另外：窗口管理器会用XselectInput 设置 SubstructureRedirectMask 来拦截窗口的创建、销毁等行为(不能被多个窗口管理器同时监听)。
     */
    XSelectInput (xdisplay, xroot, PropertyChangeMask | attrs.your_event_mask);
    XSync (xdisplay, False);

    update_wm_check (self);
}

GDWm* gd_wm_new (void)
{
    return g_object_new (GD_TYPE_WM, NULL);
}



