#include "menu-mgr.h"
#include "menu.h"

void mmgr_destroyMenuManager(mmgr_MenuManager* mmgr)
{
    mn_destroyMenu(mmgr->currentMenu);
    for(int i = 0; i < mmgr->inMenuDepth; i++)
        mn_destroyMenu(mmgr->previousMenus[i]);
}
