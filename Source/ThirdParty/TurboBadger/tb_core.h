/** @mainpage Turbo Badger - Fast UI toolkit

Turbo Badger
Copyright (C) 2011-2014 Emil Segerås

License:

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

#ifndef TB_CORE_H
#define TB_CORE_H

#include "tb_types.h"
#include "tb_hash.h"
#include "tb_debug.h"

#include "tb_widgets_listener.h"
#include "tb_linklist.h"
#include "animation/tb_widget_animation.h"

#define TB_VERSION_MAJOR 0
#define TB_VERSION_MINOR 1
#define TB_VERSION_REVISION 1
#define TB_VERSION_STR "0.1.1"

namespace tb {

class TBRenderer;
class TBSkin;
class TBWidgetsReader;
class TBLanguage;
class TBFontManager;
class TBImageManager;

struct TBCore {

TBRenderer *renderer_;
TBSkin *tb_skin_;
TBWidgetsReader *widgets_reader_;
TBLanguage *tb_lng_;
TBFontManager *font_manager_;
TBImageManager *image_manager_ = nullptr;

TBWidget *hovered_widget = nullptr;
TBWidget *captured_widget = nullptr;
TBWidget *focused_widget = nullptr;
int pointer_down_widget_x = 0;
int pointer_down_widget_y = 0;
int pointer_move_widget_x = 0;
int pointer_move_widget_y = 0;
bool cancel_click = false;
bool update_widget_states = true;
bool update_skin_states = true;
bool show_focus_state = false;

TBLinkListOf<TBWidgetListenerGlobalLink> g_listeners;

TBWidgetsAnimationManager widgets_animation_manager;

/** Initialize turbo badger. Call this before using any turbo badger API. */
bool tb_core_init(TBRenderer *renderer);

/** Shutdown turbo badger. Call this after deleting the last widget, to free turbo badger internals. */
void tb_core_shutdown();

/** Returns true if turbo badger is initialized. */
bool tb_core_is_initialized();

};

} // namespace tb

#endif // TB_CORE_H
