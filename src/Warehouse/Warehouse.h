#ifndef WAREHOUSE_H
#define WAREHOUSE_H

#include <Arduino.h>
#include <EEPROM.h>

#include "SdFat.h"

#include "warehouse.settings.h"
#include "general.settings.h"
#include "Warehouse\Ingredient\Ingredient.h"

class Warehouse{
    public:

        /**
         * @brief inizializza il magazzino, leggendo da sd o da eeprom.
        */
        Warehouse();
        Ingredient* getIngredient(const char* name);
        Ingredient* getIngredient(int index) const;
        bool isEnough(Ingredient const* ingredient, short qty);
        int getIngredientsNumber(){return _storedIngredients;}

    private:
        Ingredient _ingredients[NUM_INGREDIENTS];
        uint8_t _storedIngredients;

        void readIngredientsFromEEPROM();
        void readIngredientsFromSD();
        bool addIngredient(char *info);        
};

#endif // WAREHOUSE_H