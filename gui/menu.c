#include <assert.h>
#include "menu.h"
#include "buttons.h"
#include "parallax-bg.h"
#include "sprite.h"

mn_Menu* mn_initMenu(mn_MenuConfig* cfg, SDL_Renderer* renderer)
{
    mn_Menu* mn = malloc(sizeof(mn_Menu));
    assert(mn != NULL);

    mn->spriteCount = cfg->spriteCount;
    mn->buttonCount = cfg->buttonCount;
    mn->sprites = malloc(sizeof(spr_Sprite) * cfg->spriteCount);
    mn->buttons = malloc(sizeof(btn_Button) * cfg->buttonCount);
    mn->switchTo = NULL;
    assert(mn->sprites != NULL && mn->buttons != NULL);

    for(int i = 0; i < cfg->spriteCount; i++){
        mn->sprites[i] = spr_initSprite(&cfg->spriteCfgs[i], renderer);
    }

    for(int i = 0; i < cfg->buttonCount; i++){
        mn->buttons[i] = btn_initButton(&cfg->buttonCfgs[i], renderer);
    }

    mn->bg = bg_initBG(&cfg->bgConfig, renderer);

    return mn;
}

void mn_updateMenu(mn_Menu* mn, input_Mouse mouse)
{
    for(int i = 0; i < mn->buttonCount; i++){
        btn_updateButton(mn->buttons[i], mouse);
    }
    bg_updateBG(mn->bg);
}

void mn_drawMenu(mn_Menu* mn, SDL_Renderer* renderer, bool displayButtonHitboxes)
{
    bg_drawBG(mn->bg, renderer);

    for(int i = 0; i < mn->spriteCount; i++){
        spr_drawSprite(mn->sprites[i], renderer);
    }
    for(int i = 0; i < mn->buttonCount; i++){
        btn_drawButton(mn->buttons[i], renderer, displayButtonHitboxes);
    }

}

void mn_destroyMenu(mn_Menu* mn)
{
    for(int i = 0; i < mn->spriteCount; i++)
        spr_destroySprite(mn->sprites[i]);
    free(mn->sprites);
    mn->sprites = NULL;
    
    for(int i = 0; i < mn->buttonCount; i++)
        btn_destroyButton(mn->buttons[i]);
    free(mn->buttons);
    mn->buttons = NULL;

    bg_destroyBG(mn->bg);
    mn->bg = NULL;

    free(mn);
    mn = NULL;
}
