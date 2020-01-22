// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#include "tb_core.h"
#include "tb_skin.h"
#include "tb_widgets_reader.h"
#include "tb_language.h"
#include "tb_font_renderer.h"
#include "tb_system.h"
#include "animation/tb_animation.h"
#include "image/tb_image_manager.h"

namespace tb {

bool TBCore::tb_core_init(TBRenderer *renderer)
{
	TBDebugPrint("Initiating Turbo Badger - version %s\n", TB_VERSION_STR);
    renderer_ = renderer;
    tb_lng_ = new TBLanguage;
    font_manager_ = new TBFontManager(this);
    tb_skin_ = new TBSkin(this);
    widgets_reader_ = TBWidgetsReader::Create(this);
#ifdef TB_IMAGE
    image_manager_ = new TBImageManager(this);
#endif

    g_tb_debug.settings[TBDebugInfo::LAYOUT_BOUNDS] = true;
//    g_tb_debug.settings[TBDebugInfo::LAYOUT_PS_DEBUGGING] = true;

	return true;
}

void TBCore::tb_core_shutdown()
{
	TBAnimationManager::AbortAllAnimations();
#ifdef TB_IMAGE
    delete image_manager_;
#endif
    //FIXME SharedPtr needed here
    if (TBWidgetsReader::IsValid())
    {
        TBWidgetsReader::Clean();
    }
    delete tb_skin_;
    delete font_manager_;
    delete tb_lng_;
}

bool TBCore::tb_core_is_initialized()
{
    return widgets_reader_ ? true : false;
}

} // namespace tb
