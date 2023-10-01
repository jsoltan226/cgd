#include "menu.h"
#include "buttons.h"
#include "parallax-bg.h"
#include "sprite.h"

void mn_initMenu(mn_Menu* m, mn_PrototypeInfo* PI, SDL_Renderer* renderer)
{
    m->spriteCount = PI->spriteInfo.count;
    m->buttonCount = PI->buttonInfo.count;
    m->sprites = PI->spriteInfo.count == 0 ? NULL : malloc(sizeof(spr_Sprite) * PI->spriteInfo.count);
    m->buttons = PI->buttonInfo.count == 0 ? NULL : malloc(sizeof(btn_Button) * PI->buttonInfo.count);
    m->switchTo = NULL;

    for(int i = 0; i < PI->spriteInfo.count; i++)
    {
        m->sprites[i] = PI->spriteInfo.protos[i];
        spr_initSprite(&m->sprites[i], renderer, PI->spriteInfo.textureFilePaths[i]);
    }

    for(int i = 0; i < PI->buttonInfo.count; i++)
    {
        spr_Sprite* currentSpriteProto = &PI->buttonInfo.spriteProtos[i];
        spr_createSprite(&m->buttons[i].sprite, renderer,
                currentSpriteProto->hitbox,
                currentSpriteProto->srcRect,
                currentSpriteProto->destRect,
                PI->buttonInfo.spriteTextureFilePaths[i]);
        btn_initButton(&m->buttons[i], *currentSpriteProto, renderer, PI->buttonInfo.spriteTextureFilePaths[i]);
    }

    bg_initBG(&m->bg, renderer, PI->bgInfo.layerFilePaths, PI->bgInfo.layerSpeeds, PI->bgInfo.layerCount);
}

void mn_updateMenu(mn_Menu* m, input_Mouse mouse)
{
    for(int i = 0; i < m->buttonCount; i++){
        btn_updateButton(&m->buttons[i], mouse);
    }
    bg_updateBG(&m->bg);
}

void mn_drawMenu(mn_Menu* m, SDL_Renderer* renderer, bool displayButtonHitboxes)
{
    bg_drawBG(&m->bg, renderer);

    for(int i = 0; i < m->spriteCount; i++){
        spr_drawSprite(m->sprites[i], renderer);
    }
    for(int i = 0; i < m->buttonCount; i++){
        btn_drawButton(m->buttons[i], renderer, displayButtonHitboxes);
    }

}

void mn_destroyMenu(mn_Menu* m)
{
    for(int i = 0; i < m->spriteCount; i++)
        spr_destroySprite(&m->sprites[i]);
    free(m->sprites);
    m->sprites = NULL;
    
    for(int i = 0; i < m->buttonCount; i++)
        btn_destroyButton(&m->buttons[i]);
    free(m->buttons);
    m->buttons = NULL;

    bg_destroyBG(&m->bg);
}
