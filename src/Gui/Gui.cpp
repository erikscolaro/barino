#include "Gui.h"

void Gui::showTextCL(const char *text, uint16_t xl, uint16_t yc, int16_t h, const GFXfont *font, uint8_t size, uint16_t color, int16_t char4line)
{
    if (font!=NULL) _tft.setFont(font);
    else _tft.setFont(&FreeSans9pt7b);
    _tft.setTextSize(size);
    _tft.setTextColor(color);
    if (h==-1){ 
        h=getStrHeight();
        _tft.setCursor(xl, yc + h - size);
    } else if (h==0){
        _tft.setCursor(xl, yc+h/2);
    } else {
        _tft.setCursor(xl, yc);
    }

    char *buf=new char[strlen(text)];
    strcpy(buf, text);

    if (strlen(buf) < char4line || char4line==-1)
    {
        _tft.print(buf);
        delete(buf);
        return;
    }
    else
    {
        char *token;
        int nchar = 0;
        token = strtok(buf, " ");
        while (token != NULL)
        {
            int tokn = strlen(token);
            if (nchar + tokn <= char4line)
            {
                nchar += tokn;
                _tft.print(token);
                _tft.print(" "); // Aggiungi uno spazio tra le parole
            }
            else if (tokn > char4line)
            { // Caso in cui la parola supera la lunghezza massima per riga
                char *buf2=new char [char4line];
                size_t nc = strlcpy(buf2, token, char4line - nchar);
                _tft.print(buf2);
                _tft.print(" "); // Aggiungi uno spazio
                if ((nchar + strlen(token)) / char4line >= 2){
                    delete buf2, buf; return;
                }
                _tft.setCursor(xl, yc+h/2);
                strcpy(buf2, token + nc);
                _tft.print(buf2);
                nchar = sizeof(tokn);
                
                delete buf2;
            }
            else
            {
                if ((nchar + strlen(token)) / char4line >= 2){delete buf; return;}
                nchar = char4line;
                _tft.setCursor(xl, yc+h/2);
                _tft.print(token);
                _tft.print(" "); // Aggiungi uno spazio
                nchar += strlen(token);
            }

            token = strtok(NULL, " ");
        }
    }
    delete buf;
}

void Gui::showImageBL(const char *dir, int x, int y)
{
    SdFat SD;
    SD.begin(SD_SS, SD_SCK_MHZ(50)); //ev. sd_ss pin

    if (!SD.exists(dir)){
        Serial.print(F("[GUI] Errore apertura file! "));Serial.println(dir);
        return;
    }

    File file = SD.open(dir); //TOEND

    BitmapReader bmpReader(&file);
    uint16_t *buf = new uint16_t[bmpReader.getWidth()];

    uint16_t color;

    int16_t start_y = y - bmpReader.getHeight() + 1;
    for (int yt = y; yt >= start_y && bmpReader.readRow(buf); yt--) {
        uint16_t* buf_ptr = buf;
        for (int16_t i = 0; i < bmpReader.getWidth(); i++, buf_ptr++) {
            color = *buf_ptr;
            if (color <= CUTOFF_COLOR) {
                _tft.drawPixel(x + i, yt, color);
            }
        }
    }

    delete (buf);
    file.close();
    SD.end();
}

void Gui::drawCustomRGBBitmap(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, const uint16_t bitmap[])
{
    _tft.startWrite();
    uint16_t pixel;
    for (int16_t j = 0; j < h; j++, y++) {
        for (int16_t i = 0; i < w; i++) {
            pixel=pgm_read_word(&bitmap[j * w + i]);
                if (pixel<=0x39a7) _tft.writePixel(x + i, y, color);
        }
    }
    _tft.endWrite();
}

void Gui::showTileUL(Button* button, const char* label)
{
    int16_t x = button->getX1(),y = button->getY1(); 
    uint16_t w = button->getW(),h = button->getH(),radius = button->getRadius();
    uint16_t textbox_h = 40;
    button->drawButton();
    char buf[TILE_CHAR4LINE*2+1];
    sprintf(buf, "%s%s.bmp", IMAGES_DIR, label);
    showImageBL(buf, x + 10 , y + h - textbox_h+10);
    _tft.fillRoundRect(x, y + h - textbox_h, w, textbox_h, radius - 2, CCC);
    _tft.drawRoundRect(x, y + h - textbox_h, w, textbox_h, radius - 2, CCCC);
    showTextCL(label, x + radius / 2, y + h - textbox_h/2, textbox_h - radius, &FreeSans9pt7b, 1, CCCC, TILE_CHAR4LINE);
}

uint16_t Gui::getStrHeight(){
    int16_t x1, y1;
    uint16_t w,h;
    _tft.getTextBounds("ABC", 100, 100, &x1, &y1, &w, &h );
    Serial.println("Ho letto questo numero di altezza: "+ String(h));
    return h;
}

Gui::Gui(){
    Serial.println(F("[GUI]Called Gui builder"));

    _tft = MCUFRIEND_kbv(CS, RS, WR, RD, _RST);
    _tft.begin(_tft.readID());
    _tft.setRotation(TOUCH_ORIENTATION);

    Serial.println(F("[GUI]_tft object correctly initialized"));

    _recipesNum=0;

    SdFat SD;
    SD.begin();

    File recipesFolder=SD.open(RECIPES_DIR);
    if (!recipesFolder) Serial.println(F("[GUI]Recipes folder not opened"));
    else Serial.println(F("[GUI]Recipes filename list:"));

    File activeRecipe;

    while (activeRecipe.openNext(&recipesFolder, O_RDONLY)) {
        activeRecipe.printName(&Serial);
        Serial.println();
        if (activeRecipe.isFile() && !activeRecipe.isHidden()) _recipesNum++;
        activeRecipe.close();
    }

    recipesFolder.close();
    SD.end();

    Serial.print(F("[GUI] Files counted:"));
    Serial.println(_recipesNum);

    uiStatus._actual=BEGIN;
    requestTransition(STATE_HOMEPAGE);

    _homepage=Homepage(this);
    _drinkPage=DrinkPage(this);
    _settingsPage=SettingsPage(this);
    _executionPage=ExecutionPage(this);
    

    requestRefresh();
}

void Gui::show()
{

    Serial.print(F("[GUI]Gui status:\n[_actual] "));
    Serial.print(uiStatus._actual);
    Serial.print(F(" \t[_next] "));
    Serial.print(uiStatus._next);
    Serial.print(F("\t[_requestrefresh] : "));
    Serial.println(uiStatus._refreshReq);

    State status;
    if (requestedTransition()){
        status = uiStatus._next;
    } else if (requestedRefresh()){
        status = uiStatus._actual;
    } else {
        return; //nothing to update
    }
    
    switch (status){
       case STATE_HOMEPAGE:
           _homepage.show();
           break;
       case STATE_DRINK:
           _drinkPage.show();
           break;
       case STATE_EXECUTER:
           _executionPage.show();
           break;
       case STATE_SETTINGS:
           _settingsPage.show();
           break;
       case BEGIN:
           //power on screen...
           _tft.fillScreen(C_BACK);
           _tft.println("Beginning gui...");
           requestTransition(STATE_HOMEPAGE);
           break;
       default:
           Serial.print(F("[GUI]Error: transition to state "));
           Serial.print(uiStatus._next);
           Serial.println(F("not implemented."));
           requestTransition(BEGIN);
           return;
    }
    completeTransition();
    completeRefresh();
}

bool Gui::interact(int xcc, int ycc)
{
    switch (uiStatus._actual){
        case STATE_HOMEPAGE:
            return _homepage.interact(xcc,ycc);
        case STATE_DRINK:
            return _drinkPage.interact(xcc,ycc);
        case STATE_EXECUTER:
            return _executionPage.interact(xcc,ycc);
        case STATE_SETTINGS:
            return _settingsPage.interact(xcc,ycc);
        case BEGIN: 
            requestTransition(STATE_HOMEPAGE);
            return true;
        default:
            Serial.print( F("[GUI] Error: interaction with "));
            Serial.print(uiStatus._next);
            Serial.println(F(" not implemented."));
            requestTransition(BEGIN);
            return false;
    }
}

Gui::Homepage::Homepage(Gui *gui):_gui(gui){
    _pagenum=_gui->_recipesNum/TILE4PAGE;
    if (_gui->_recipesNum%TILE4PAGE>0) _pagenum++;
    _pagei=0;

    uint16_t tile_spacing = 10, side_spacing = 10, radius = 10;
    uint16_t tw = (320 - side_spacing * 2 - tile_spacing * (TILE4ROW - 1)) / TILE4ROW;
    uint16_t th = (480 - 50 - 2 * side_spacing - tile_spacing * (TILE4COL - 1)) / TILE4COL;
    uint16_t x, y;

    for (int i=0; i< TILE4PAGE; i++){
        x = side_spacing + (tw + tile_spacing) * (i % TILE4ROW);
        y = side_spacing + (th + tile_spacing) * (i / TILE4ROW);
        drinkButtons[i].initButtonUL(
            &gui->_tft, x, y, tw, th, radius, CCC, WHITE, TRANSPARENT, NULL, 1
        );
    }

    const uint16_t button_spacing = 5, bottom_spacing = 5, upper_spacing = 5;
    uint16_t bw = (275 - 2 * side_spacing - button_spacing * (_pagenum - 1)) / _pagenum;
    uint16_t bh = 50 - upper_spacing - bottom_spacing;
    radius = (bh < bw ? bw : bh) / 2;
    y=430 + upper_spacing;

    for (int i = 0; i < _pagenum; i++)
    {
        x = side_spacing + (bw + button_spacing) * i;
        navigationButtons[i].initButtonUL(&gui->_tft, x, y, bw, bh, radius, i == _pagei ? CCCC : C, CC, CCCC, numbers+i*2 ,1);
    }
    settingsButton.initButtonUL(&gui->_tft, 275, 435, 40, 40, 10, CC, CC, C, "", 1);
}

void Gui::Homepage::show()
{
    _gui->_tft.setFont(&FreeSans9pt7b);
    _gui->_tft.fillScreen(C_BACK);

    //stores names relative to the _pagei into _gui->_recipeNames
    SdFat SD;
    SD.begin(SD_SS, SPI_FULL_SPEED);

    File recipesFolder=SD.open(RECIPES_DIR), activeRecipe;
    if (!recipesFolder) Serial.println(F("[GUI]Recipes folder not opened"));
    int openedFiles=0, idx = 0;
    while (idx < TILE4PAGE && activeRecipe.openNext(&recipesFolder, O_RDONLY)) {

        if (activeRecipe.isFile() && !activeRecipe.isHidden()){
            if (idx >= _pagei*TILE4PAGE){
                activeRecipe.getName(_gui->_recipesNames[idx], sizeof(_gui->_recipesNames[idx]));
                //remove the extension
                _gui->_recipesNames[idx][strlen(_gui->_recipesNames[idx])-4]='\0';
                idx++;
            }
            openedFiles++;
        }
        activeRecipe.close();
    }

    recipesFolder.close();
    SD.end();

    if (_gui->requestedTransition()) {
        _gui->_tft.fillScreen(C_BACK);
        _gui->_tft.fillRect(0, 430, 320, 50, C);
        for (int i = 0; i < _pagenum; i++) navigationButtons[i].drawButton();
        settingsButton.drawButton();
        _gui->drawCustomRGBBitmap(275, 435, 40, 40, CCCC ,settings_bmp);
    }

    for (int i=0; i<min(TILE4PAGE, _gui->_recipesNum-TILE4PAGE*_pagei); i++){
        _gui->showTileUL(&(drinkButtons[i]), _gui->_recipesNames[i]);
    }

}

bool Gui::Homepage::interact(int xcc, int ycc)
{
    for (int i=0; i<TILE4PAGE; i++){
        if (drinkButtons[i].contains(xcc, ycc)){
            _gui->setSelectedRecipeName(_gui->_recipesNames[i]);
            _gui->requestTransition(STATE_DRINK);
            return true;
        }
    }

    for (int i = 0; i < _pagenum; i++)
    {
        if (navigationButtons[i].contains(xcc, ycc)){
            _pagei=(_pagei+1)%_pagenum;
            _gui->requestRefresh();
            return true;
        }
    }

    if (settingsButton.contains(xcc, ycc)){
        _gui->requestTransition(STATE_SETTINGS);
        return true;
    }

    return false;
}

Gui::SettingsPage::SettingsPage(Gui *gui):_gui(gui)
{
}

void Gui::SettingsPage::show()
{
    _gui->_tft.fillScreen(C_BACK);

    int x=5, y=15, h=8, w=100;
    int16_t i=0, aqty, mqty;
    char buffer[BUF_LEN];
    _gui->_tft.setFont(&FreeSans9pt7b);
    _gui->_tft.setTextSize(1);
    Ingredient* tempIng;
    for( int i=0; i<_gui->_warehouse.getIngredientsNumber(); i++){
        tempIng=_gui->_warehouse.getIngredient(i);
        aqty=tempIng->getQuantity();
        mqty=tempIng->getMaxQuantity();
        _gui->_tft.setCursor(x,y);
        sprintf(buffer, "%02d. %-10.10s", i+1, tempIng->getName());
        _gui->_tft.print(buffer);
        _gui->_tft.drawRect(215, y-12, 104, 13, CCCC);
        _gui->_tft.fillRect(217, y-10, (aqty/mqty*100), 9, (aqty/mqty*100)>50?CCCC:((aqty/mqty*100)>25?TFT_ORANGE:TFT_RED));
        _gui->_tft.setCursor(165,y);
        sprintf(buffer, "%3d%%", aqty/mqty*100);
        _gui->_tft.print(buffer);
        y+=15;
    }
}

bool Gui::SettingsPage::interact(int xcc, int ycc)
{
    _gui->requestTransition(STATE_HOMEPAGE);
    return true;
}

Gui::ExecutionPage::ExecutionPage(Gui *gui): _gui(gui)
{
    //TODO
}

void Gui::ExecutionPage::show()
{
    //TODO
    _gui->_tft.fillScreen(C_BACK);
    Recipe * r = _gui->getSelectedRecipeObj();

    _gui->_tft.setFont(&FreeSans9pt7b);
    _gui->_tft.setCursor(10,20);
    _gui->_tft.println(r->getName());
    for (Recipe::StepIterator it = r->begin(); it!=r->end(); ++it){
        _gui->_tft.print((char) it->getAction());_gui->_tft.print("  ");
        if (it->getAction()==ADD) {
            _gui->_tft.print(it->getIngredient()->getName());_gui->_tft.print("  ");
            _gui->_tft.println(it->getModQty());
        } else _gui->_tft.println(" ");
    }

    _gui->showPopup("Non ancora implementato. Clicca per riavviare.");
}

bool Gui::ExecutionPage::interact(int xcc, int ycc)
{
    _gui->requestTransition(STATE_HOMEPAGE);
    return true;
}

Gui::DrinkPage::DrinkPage(Gui *gui): _gui(gui)
{
    MCUFRIEND_kbv* tftptr = _gui->getTftptr();
    uint16_t x,y,h,w,r;
    x=9, y=65, w=50, h=60,r=25;
    this->_back.initButtonUL(tftptr,x,y+5,w,h-10,r,CCCC,TFT_ORANGE,CCCC,nullptr,1);
    x+=w+13, r=10;
    this->_small.initButtonUL(tftptr,x,y,w,h,r, CCCC, WHITE, CCCC, nullptr, 1);
    x+=w+13;
    this->_medium.initButtonUL(tftptr,x,y,w,h,r, CCCC, WHITE, CCCC, nullptr, 1);
    this->_medium.press();
    x+=w+13;
    this->_large.initButtonUL(tftptr,x,y,w,h,r, CCCC, WHITE, CCCC, nullptr, 1);
    x+=w+13, r=25;
    this->_forward.initButtonUL(tftptr,x,y+5,w,h-10,r, CCCC, TFT_OLIVE, CCCC, nullptr, 1);

    x+=10, y=155, h=55, w=55, r=15;
    for (int i=0; i<SETTINGS_MAX_NUM; i++){
        x=145; 
        this->_settings[i][0].initButtonUL(tftptr, x, y, w, h, r, CCCC, CC, CCCC, nullptr, 3);
        x=245;
        this->_settings[i][1].initButtonUL(tftptr, x, y, w, h, r, CCCC, CC, CCCC, nullptr, 3);
        y+=h+10;
    }
}
    
void Gui::DrinkPage::show()
{
    //new text
    Recipe* selRecipe = _gui->getSelectedRecipeObj();
    MCUFRIEND_kbv* tftptr = _gui->getTftptr();
    uint16_t x,y,h,w,r;

    if (_gui->requestedTransition()){ //complete show
        tftptr->fillScreen(C_BACK);

        char buf[BUF_LEN];
        sprintf(buf, "%.10s\0", selRecipe->getName());
        _gui->showTextCL(buf, 9, 20, -1, &FreeSansBold12pt7b, 2, CCCC, -1);

        x=9, y=65, w=50, h=60, r=25;
        _gui->drawCustomRGBBitmap(x,y,w,h,CCCC,back_bmp);
        x+=w+13;
        _small.unpress();
        _small.drawButton();
        _gui->drawCustomRGBBitmap(x,65,w,h, _small.isPressed()?CCCC:CCC, shot_bmp);
        x+=w+13;
        _medium.press();
        _medium.drawButton();
        _gui->drawCustomRGBBitmap(x,65,w,h, _medium.isPressed()?CCCC:CCC, medium_bmp);
        x+=w+13;
        _large.unpress();
        _large.drawButton();
        _gui->drawCustomRGBBitmap(x,65,w,h, _large.isPressed()?CCCC:CCC, large_bmp);
        x+=w+13;
        _forward.drawButton();
        _gui->drawCustomRGBBitmap(x,y,w,h,CCCC,ok_bmp);

        x=10, y+=10+h;
        tftptr->drawFastHLine(x, y, tftptr->width()-x*2, CCCC);
    
        x=10, y=145, r=15;
        tftptr->fillRoundRect(x, y, tftptr->width()-x*2, tftptr->height(), r, C_BACK);
        tftptr->drawRoundRect(x, y, tftptr->width()-x*2, tftptr->height(), r, CCCC);

        x+=10, y+=10, h=55, w=55;
        int i=0, printed=0; 

        for( Recipe::StepIterator r = selRecipe->begin(); r!=selRecipe->end() && printed<SETTINGS_MAX_NUM; ++r){
            if (r->getAction()==ADD && r->getIngredient()->isEditable()){
                //label
                x=20;
                sprintf(buf, "%.10s%s\0", r->getIngredient()->getName(), strlen(r->getIngredient()->getName())>10?"...":"");
                _gui->showTextCL(buf, x, y+h/2, -1, &FreeSansBold12pt7b, 1, CCCC, -1);

                //qty
                x=145+w+10;
                sprintf(buf, "%d\0", r->getModQty());
                _gui->showTextCL(buf, x, y+h/2, -1, &FreeSansBold12pt7b, 1, CCCC, -1);

                //buttons
                x=145;
                _settings[printed][0].drawButton();
                _gui->drawCustomRGBBitmap(x,y,w,h,CCCC,minus);
                x=245;
                _settings[printed][1].drawButton();
                _gui->drawCustomRGBBitmap(x,y,w,h,CCCC,plus);

                y+=h+10;
                printed++;
            }
            i++;
        }

    } else if (_gui->requestedRefresh()){ //only refresh
        char buf[BUF_LEN];

        x=9, y=65, w=50, h=60, r=25;
        x+=w+13;
        if (_small.justChanged()){
            _small.drawButton();
            _gui->drawCustomRGBBitmap(x,65,w,h, _small.isPressed()?CCCC:CCC, shot_bmp);
        }
        x+=w+13;
        if (_medium.justChanged()){
            _medium.drawButton();
            _gui->drawCustomRGBBitmap(x,65,w,h, _medium.isPressed()?CCCC:CCC, medium_bmp);
        }
        x+=w+13;
        if (_large.justChanged()){
            _large.drawButton();
            _gui->drawCustomRGBBitmap(x,65,w,h, _large.isPressed()?CCCC:CCC, large_bmp);
        }
        x=20, y=155, h=55, w=55, r=15;

        int i=0, printed=0; 
        for( Recipe::StepIterator r = selRecipe->begin(); r!=selRecipe->end() && printed<SETTINGS_MAX_NUM; ++r){
            if (r->getAction()==ADD && r->getIngredient()->isEditable()){
                //buttons
                if (_settings[printed][0].justChanged()){ //minus
                    bool thereis = selRecipe->addIngredientQty(r->getIngredient(), r->getIngredient()->isLiquid()?-10:-1);
                    sprintf(buf, "%d\0", selRecipe->getIngredientRequiredQty(r->getIngredient()));
                    _gui->_tft.fillRect(145+w+1, y, w-15,h, C_BACK);
                    _gui->showTextCL(buf, 145+w+10, y+h/2, -1, &FreeSansBold12pt7b, 1, thereis?CCCC:TFT_RED, -1);
                    break;
                } else if (_settings[printed][1].justChanged()){ //plus
                    bool thereis = selRecipe->addIngredientQty(r->getIngredient(), r->getIngredient()->isLiquid()?10:1);
                    sprintf(buf, "%d\0", selRecipe->getIngredientRequiredQty(r->getIngredient()));
                    _gui->_tft.fillRect(145+w+1, y, w-15,h, C_BACK);
                    _gui->showTextCL(buf, 145+w+10, y+h/2, -1, &FreeSansBold12pt7b, 1, thereis?CCCC:TFT_RED, -1);
                    break;                
                }

                y+=h+10;
                printed++;
            }
            i++;
        }
    }
}

bool Gui::DrinkPage::interact(int xcc, int ycc)
{
    Recipe* selRecipe = &_gui->uiStatus._activeRecipe;
    if (_back.contains(xcc,ycc)) {
        _gui->requestTransition(STATE_HOMEPAGE);
        return true;
    } else if (_forward.contains(xcc,ycc)){
        if (_large.isPressed()) selRecipe->adjustTotalVolume(DrinkCapacity::LARGE);
        else if (_medium.isPressed()) selRecipe->adjustTotalVolume(DrinkCapacity::MEDIUM);
        else if (_small.isPressed()) selRecipe->adjustTotalVolume(DrinkCapacity::SMALL);
        else {
            _gui->showPopup("ERRORE: seleziona la dimensione di un cocktail per procedere.");
            return true;
        }
        if (selRecipe->checkEnoughIngredientsInWarehouse()){
            _gui->requestTransition(STATE_EXECUTER);
        } else {
            _gui->showPopup("Errore: non ci sono sufficienti ingredienti. Prova a modificare qualcosa o cambia cocktail.");
        }
        return true;
    } else if (_small.contains(xcc,ycc)){
        _small.press();
        _medium.unpress();
        _large.unpress();
        _gui->requestRefresh();
        return true;
    } else if (_medium.contains(xcc,ycc)){
        _small.unpress();
        _medium.press();
        _large.unpress();
        _gui->requestRefresh();
        return true;
    } else if (_large.contains(xcc,ycc)){
        _small.unpress();
        _medium.unpress();
        _large.press();
        _gui->requestRefresh();
        return true;
    } else {
        for (int i=0; i<SETTINGS_MAX_NUM; i++){
            if (this->_settings[i][0].contains(xcc,ycc)){
                this->_settings[i][0].press();
                _gui->requestRefresh();
                return true;
            }

            if (this->_settings[i][1].contains(xcc,ycc)){
                this->_settings[i][1].press();
                _gui->requestRefresh();
                return true;
            }
        }    
    }
    return false;
}

void Gui::setSelectedRecipeName(char *recipeName)
{
    SdFat SD;
    SD.begin(SD_SS, SD_SCK_MHZ(16));
    char buf[RECIPE_NAME_LEN+10];
    sprintf(buf, "%s/%s.csv", RECIPES_DIR, recipeName);
    File f = SD.open(buf);
    uiStatus._activeRecipe=Recipe(&f, &_warehouse);
    f.close();
    SD.end();
}

Recipe *Gui::getSelectedRecipeObj()
{
    return &uiStatus._activeRecipe;
}

void Gui::requestTransition(State newState)
{
    uiStatus._next=newState;
}

bool Gui::requestedTransition()
{
    return uiStatus._actual!=uiStatus._next;
}

void Gui::completeTransition()
{
    uiStatus._actual=uiStatus._next;
}

void Gui::requestRefresh()
{
    uiStatus._refreshReq=true;
}

void Gui::completeRefresh()
{
    uiStatus._refreshReq=false;
}

void Gui::showPopup(char *error)
{
    _tft.fillRoundRect(20, _tft.height()/5*2, _tft.width()-40, _tft.height()/5, 20, C);
    _tft.drawRoundRect(20, _tft.height()/5*2, _tft.width()-40, _tft.height()/5, 20, CCCC);
    showTextCL(error, 25, _tft.height()/2, 0 , &FreeSans9pt7b, 1, CCCC, 25);
    delay(2000);

    requestTransition(uiStatus._actual);
    uiStatus._actual=ERROR;

    show();
}

bool Gui::requestedRefresh()
{
    return uiStatus._refreshReq;
}
