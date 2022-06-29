#include <math.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_primitives.h>

#include "list.h"

#pragma warning(disable : 4996)

//Constantes que se usarán en todo el código, porque, por ejemplo, tener PYRO es mucho mejor que tener 6000 sabes?
#define NO_AURA 0                   //Cuando no hay aura
#define PYRO 6000                   //ID del aura Pyro
#define ELECTRO 6010                //ID del aura Electro
#define CRYO 6020                   //ID del aura Cryo
#define HYDRO 6030                  //ID del aura Hydro
#define ANEMO 6040                  //ID del aura Anemo

#define AURA_TAX 0.8                //Impuesto por imbuir con un aura
#define WEAK_AMP_MODIFIER 0.5       //Modificador elemental por una reacción amplificadora débil
#define STRONG_AMP_MODIFIER 2       //Modificador elemental por una reacción amplificadora fuerte
#define OVERLOAD_MODIFIER 1         //Modificador elemental por la reacción transformativa Sobrecarga
#define SWIRL_MODIFIER 0.5          //Modificador elemental por la reacción transformativa Torbellino
#define ELECTROCHARGED_MODIFIER 0.4 //Modificador elemental por la reacción transformativa Electrocargado

#define WEAK_AMP_MULTIPLIER 1.5        //Bonificación por una reacción amplificadora débil
#define STRONG_AMP_MULTIPLIER 2        //Bonificación por una reacción amplificadora fuerte
#define OVERLOAD_MULTIPLIER 4          //Bonificación por gatillar Sobrecarga
#define ELECTROCHARGED_MULTIPLIER 2.4  //Bonificación por gatillar Electrocargado
#define SWIRL_MULTIPLIER 1.2           //Bonificación por gatillar Torbellino
#define SUPERCONDUCT_MULTIPLIER 1      //Bonificación por gatillar Superconductor

#define BUFF_MULTIPLIER 1.3         //Multiplicador por buffar una estadística
#define DEBUFF_MULTIPLIER 0.7       //Multiplicador por debuffar una estadística



/*
* ALLEGRO_COLOR al_map_rgba_f(float r, float g, float b, float a) 
* sirve para que algo sea trasl�cido con el a = alpha
* (se podr�a usar esto para oscurecer a los personajes cuando no est�n hablando?)
*/

/* para que el codigo sea mas conciso se crea esta funcion para que revise si las cosas se inicializaron bien
 * se aplica el codigo para cada chequeo
*/

/* se deberia crear una lista de menus, para asi solo ir yendo entre los menus que queremos
*  o que sea un arbol e ir navegando entre menus como si navegaramos en el arbol o mapa o como sea
*/
typedef struct MENU
{
    List* options;
    int selectedOption;
    int size;
}MENU;

typedef struct wordData
{
    char const* text;           //texto
    float x;                    //coordenada X
    float y;                    //coordenada Y
    float flags;                //flag de la palabra
    bool selected;              //si se encuentra seleccionada o no (CUIDADO QUE PUEDE QUE NUNCA HAYA USADO ESTO REALMENTE)
}wordData;

typedef struct Player
{
    List* characters;
    int turns;
    int AliveCharacters;
}Player;

/*
 * U: original notation
 * GU: Elemental Gauge of source
 * GUs cannot go below zero
 *
 * When an elemental ability is used against an enemy the resulting elemental
 * aura is:
 *
 * xU * 0.8 = totalGU
 *
 * Unit modifiers to gauge consuption:
 *
 * Melt and Vaporize:
 * Weak amping elemental triggers have a 0.5x modifier.
 * Example:An enemy affected by Amber’s Charged Shot has 1.6B Pyro.
 * Using Kaeya’s E (2B Cryo) only removes 1GU Pyro because weak
 * melt occurs when the trigger is Cryo.
 *
 * Strong amping elemental triggers have a 2x modifier.
 *
 * Example: An enemy affected by Kaeya’s E has 1.6B Cryo.
 * Using Diluc’s E (1A) removes 2GU worth of Cyro aura because strong
 * melt occurs when the trigger is Pyro. This leaves us
 * with 0GU Cryo as gauges cannot go below zero.
 *
*/
typedef struct ELEMENTAL_AURA
{
    int ID;     //identificador
    float U;    //notación original
    float GU;    //notación de reacción
    //float U;    //Ratio de decaída: antes de esto veamos como le va a lo otro
    //int TypeOfDecay   //igual asi?
}ELEMENTAL_AURA;

typedef struct Objective_Lists
{
    List* sack;
    size_t quantitySack;   //equivale a la cantidad de miembros en el equipo
}Objective_Lists;

typedef struct ENEMY
{
    int ID;
    int LVL;
    float HP;    //vida
    float TotalHP;
    float ATK;   //ataque
    float FinalATK;
    float DEF;   //defensa: por fórmula creo que no es necesaria
    float FinalDEF;
    int turns;
    int PosHPX;
    int PosHPY;
    List* Abilities;
    ELEMENTAL_AURA* AuraApplied;
    Objective_Lists* objectives;
    int turnATKBuffCounter;
    int turnDEFBuffCounter;
}ENEMY;

//sabes que estaria guapo para guardar memoria? que hubiese un union aqui dentro
typedef struct ABILITY
{
    char* name;             //nombre porque me cago en la puta
    int ID;                 //muestra el tipo de relacion con el entorno
    //char * nombre;        //para mostrar en el menú de selección?
    bool active;            //checkea si la habilidad esta activa
    float Multiplier;       //serviria tanto para buffos como para ataques?
    int CDTurns;            //what the fuck
    int repetitions;        //Cantidad de veces que la habilidad hará efecto
    int repetitionCounter;
    int TurnsUntilEffect;   //Cantidad de turnos que le tomará para hacer efecto de nuevo
    int turnCounter;
    ELEMENTAL_AURA* aura;
    int posX;
    int posY;
    bool selected;
}ABILITY;

typedef struct CHARACTER
{
    char* name;                     //nombre porque me cago en la puta
    int ID;                         //identificador, debería ser constante o no es así como funcionan?
    int LVL;                        //queremos niveles realmente?
    float CurrentHP;                //vida actual
    float TotalHP;                  //vida maxima
    float ATK;                      //ataque
    float FinalATK;                 //ataque final
    float DEF;                      //defensa
    float FinalDEF;                 //defensa final
    int EM;                         //maestria elemental
    int elementID;                  //elemento al que pertenece, debería ser constante o no es así como funcionan?
    List* Abilities;                //lista de habilidades
    int AmountOfAbilities;          //cantidad de habilidades
    ELEMENTAL_AURA* AuraApplied;    //aura aplicada en si mismo
    bool selected;
    int posHPX;                     //posicion X de la vida en la pantalla?
    int posHPY;                     //posicion Y de la vida en la pantalla?
    MENU* menuHabilidades;          //info de los datos para presentar el menu y esas cosas
}CHARACTER;

/*igual se deberia cambiar a habilidades activas?*/
typedef struct ACTION
{
    CHARACTER* character;
    ABILITY* abilityUsed;
}ACTION;




void must_init(bool test, const char* description)
{
    if (test) return;

    printf("couldn't initialize %s\n", description);
    exit(1);
}

const char* get_csv_field(char* tmp, int k)
{
    int open_mark = 0;
    char* ret = (char*)malloc(100 * sizeof(char));
    if (ret == NULL)
    {
        exit(1);
    }
    int ini_i = 0, i = 0;
    int j = 0;
    while (tmp[i + 1] != '\0') {

        if (tmp[i] == '\"')
        {
            open_mark = 1 - open_mark;
            if (open_mark) ini_i = i + 1;
            i++;
            continue;
        }

        if (open_mark || tmp[i] != ',')
        {
            if (k == j) ret[i - ini_i] = tmp[i];
            i++;
            continue;
        }

        if (tmp[i] == ',')
        {
            if (k == j)
            {
                ret[i - ini_i] = 0;
                return ret;
            }
            j++; ini_i = i + 1;
        }

        i++;
    }

    if (k == j)
    {
        ret[i - ini_i] = 0;
        return ret;
    }

    return NULL;
}

List* importMenuData(const char* arch, int cantElementos)
{
    int campo;
    char linea[1024];
    List* menu = createList();
    FILE* fp = fopen(arch, "r");

    while (fgets(linea, 1024, fp) != NULL)
    {
        wordData* word = (wordData*)malloc(sizeof(wordData));
        if (word == NULL)
        {
            exit(1);
        }

        for (campo = 0; campo < 5; campo += 1)
        {
            char* aux = (char*)get_csv_field(linea, campo);

            if (campo == 0)
            {
                //texto
                word->text = aux;
                //printf("se ha insertado texto: %s\n", word->text);
            }
            if (campo == 1)
            {
                // X
                word->x = atoi(aux);
                //printf("se ha insertado LVL: %i\n", character->LVL);
            }
            if (campo == 2)
            {
                // Y
                word->y = atoi(aux);
                //printf("se ha insertado la vida: %f\n", character->HP);
            }
            if (campo == 3)
            {
                // Flags
                word->flags = atoi(aux);
                //printf("se ha insertado ATK: %f\n", character->ATK);
            }
        }

        //por algun motivo si se pone pushBack solo se guardan el Game start y el load Game
        pushFront(menu, word);
    }

    //está para confirmar los datos
    wordData* wordAux = firstList(menu);
    for (int i = 0; i < cantElementos; i++)
    {
        printf("%s\n", wordAux->text);
        wordAux = nextList(menu);
    }
    return menu;
}

List* Import_Team_Archive(char* teamArchive)
{
    char lineaPersonaje[1024];
    char linea[1024];
    int campoPersonajes;
    int data;
    int characterCounter = 0; //posible inutil
    int cont = 0;
    List* CharacterList = createList();


    FILE* equipo = fopen(teamArchive, "r");
    while (fgets(lineaPersonaje, 1024, equipo) != NULL)
    {
        for (campoPersonajes = 0; campoPersonajes < 4; campoPersonajes += 1)
        {
            char* archivoPersonaje = (char*)get_csv_field(lineaPersonaje, campoPersonajes);
            FILE* fp = fopen(archivoPersonaje, "r");
            cont = 1;

            /*Se reserva memoria*/
            CHARACTER* character = (CHARACTER*)malloc(sizeof(CHARACTER));
            if (character == NULL)
            {
                exit(1);
            }
            character->AuraApplied = (ELEMENTAL_AURA*)malloc(sizeof(ELEMENTAL_AURA));
            if (character->AuraApplied == NULL)
            {
                exit(1);
            }
            character->Abilities = createList();
            character->menuHabilidades = (MENU*)malloc(sizeof(MENU));

            while (fgets(linea, 1024, fp) != NULL)
            {
                //printf("cont = %i\n", cont);
                if (cont == 1)
                {
                    //printf("entra en cont 1\n");
                    for (data = 0; data < 14; data += 1)
                    {
                        //printf("va a guardar los datos del personaje");
                        char* aux = (char*)get_csv_field(linea, data);
                        if (data == 0)
                        {
                            //ID DEL PERSONAJE
                            character->name = aux;
                            printf("se ha insertado el nombre: %s\n", character->name);
                        }
                        if (data == 1)
                        {
                            //ID DEL PERSONAJE
                            character->ID = atoi(aux);
                            printf("se ha insertado ID: %i\n", character->ID);
                        }
                        if (data == 2)
                        {
                            // NIVEL
                            character->LVL = atoi(aux);
                            printf("se ha insertado LVL: %i\n", character->LVL);
                        }
                        if (data == 3)
                        {
                            // PUNTOS DE VIDA
                            character->CurrentHP = atoi(aux);
                            printf("la vida actual es: %f\n", character->CurrentHP);
                        }
                        if (data == 4)
                        {
                            character->TotalHP = atoi(aux);
                            printf("la vida máxima es: %f\n", character->TotalHP);
                        }
                        if (data == 5)
                        {
                            //ATAQUE
                            character->ATK = atoi(aux);
                            printf("se ha insertado ATK: %f\n", character->ATK);
                        }
                        if (data == 6)
                        {
                            character->FinalATK = atoi(aux);
                            printf("el ataque final es: %f\n", character->FinalATK);
                        }
                        if (data == 7)
                        {
                            //DEFENSA
                            character->DEF = atoi(aux);
                            printf("se ha insertado DEF: %f\n", character->DEF);
                        }
                        if (data == 8)
                        {
                            character->FinalDEF = atoi(aux);
                            printf("la defensa final es: %f\n", character->FinalDEF);
                        }
                        if (data == 9)
                        {
                            //MAESTRÍA ELEMENTAL
                            character->EM = atoi(aux);
                            printf("se ha insertado EM: %i\n", character->EM);
                        }
                        if (data == 10)
                        {
                            /*ID DEL ELEMENTO*/
                            character->elementID = atoi(aux);
                            printf("se ha insertado el Element ID: %i\n", character->elementID);
                        }
                        if (data == 11)
                        {
                            //printf("entra en data 10\n");
                            character->AmountOfAbilities = atoi(aux);
                            printf("el personaje tiene %i habilidades\n", character->AmountOfAbilities);
                        }
                        if (data == 12)
                        {
                            character->posHPX = atoi(aux);
                            printf("la pos X de la vida es: %i\n", character->posHPX);
                        }
                        if (data == 13)
                        {
                            character->posHPY = atoi(aux);
                            printf("la pos Y de la vida es: %i\n", character->posHPY);
                        }
                        /*
                        if (data == 14)
                        {
                            character->menuHabilidades->options = importMenuData(aux,character->AmountOfAbilities);
                            printf("se ha importado el menu de las habilidades\n");
                        }*/
                    }
                }
                if (cont > 1)
                {
                    /*se reserva memoria para la habilidad y el aura de esta*/
                    ABILITY* abilityAux = (ABILITY*)malloc(sizeof(ABILITY));
                    if (abilityAux == NULL)
                    {
                        exit(1);
                    }

                    abilityAux->aura = (ELEMENTAL_AURA*)malloc(sizeof(ELEMENTAL_AURA));
                    if (abilityAux->aura == NULL)
                    {
                        exit(1);
                    }

                    for (data = 0; data < 14; data += 1)
                    {
                        char* aux = (char*)get_csv_field(linea, data);
                        if (data == 0)
                        {
                            /*ESTADO DE LA HABILIDAD*/
                            abilityAux->ID = atoi(aux);
                            printf("El ID de la habilidad es: %i\n", abilityAux->ID);
                        }
                        if (data == 1)
                        {
                            /*ESTADO DE LA HABILIDAD*/
                            abilityAux->active = atoi(aux);
                            printf("El estado de la habilidad es: %i\n", abilityAux->active);
                        }
                        if (data == 2)
                        {
                            /*ESTADO DE LA HABILIDAD*/
                            abilityAux->Multiplier = atoi(aux);
                            printf("El multiplicador de la habilidad es: %f\n", abilityAux->Multiplier);
                        }
                        if (data == 3)
                        {
                            /*ESTADO DE LA HABILIDAD*/
                            abilityAux->CDTurns = atoi(aux);
                            printf("Los turnos de CD son: %i\n", abilityAux->CDTurns);
                        }
                        if (data == 4)
                        {
                            /*ESTADO DE LA HABILIDAD*/
                            abilityAux->repetitions = atoi(aux);
                            printf("Las repeticiones que tendra la habilidad son: %i\n", abilityAux->repetitions);
                        }
                        if (data == 5)
                        {
                            /*ESTADO DE LA HABILIDAD*/
                            abilityAux->repetitionCounter = atoi(aux);
                            printf("El contador de repeticiones esta en: %i\n", abilityAux->repetitionCounter);
                        }
                        if (data == 6)
                        {
                            /*ESTADO DE LA HABILIDAD*/
                            abilityAux->TurnsUntilEffect = atoi(aux);
                            printf("La habilidad hara efecto cada %i turnos\n", abilityAux->TurnsUntilEffect);
                        }
                        if (data == 7)
                        {
                            /*ESTADO DE LA HABILIDAD*/
                            abilityAux->turnCounter = atoi(aux);
                            printf("El contador de turnos de la habilidad esta en: %i\n", abilityAux->turnCounter);
                        }
                        if (data == 8)
                        {
                            abilityAux->aura->ID = atoi(aux);     //se podría usar el ID del elemento del personaje pero que paja
                            printf("el ID elemental de la habilidad es: %i\n", abilityAux->aura->ID);
                        }
                        if (data == 9)
                        {
                            abilityAux->aura->U = atoi(aux);
                            printf("los U de la habilidad es: %f\n", abilityAux->aura->U);
                        }
                        if (data == 10)
                        {
                            abilityAux->aura->GU = atoi(aux);
                            printf("los GU de la habilidad es: %f\n", abilityAux->aura->GU);
                        }
                        /*coordenadas y flag de seleccionada o no*/
                        if (data == 11)
                        {
                            abilityAux->posX = atoi(aux);
                            printf("la coordenada X es: %i\n", abilityAux->posX);
                        }
                        if (data == 12)
                        {
                            abilityAux->posY = atoi(aux);
                            printf("la coordenada Y es: %i\n", abilityAux->posY);
                        }
                        if (data == 13)
                        {
                            abilityAux->name = aux;
                            printf("el nombre de la habilidad es: %s\n", abilityAux->name);
                        }
                    }

                    pushFront(character->Abilities, abilityAux);
                    //printf("se ha insertado una habilidad\n\n");
                }
                cont += 1;
                //listaGeneral->capacidad += 1;
            }

            //printf("se llena el aura del personaje\n");
            character->AuraApplied->ID = 0;
            character->AuraApplied->U = 0;
            character->AuraApplied->GU = 0;

            //se coloca el estado de inserción del personaje
            character->selected = false;

            //printf("se inserta el personaje en la lista\n");
            pushFront(CharacterList, character);
            printf("se ha importado %i\n", character->ID);
        }
    }

    return CharacterList;  //cuidado con los punteros
}

/*Le queremos pasar la cantidad de habilidades al enemigo para hacer las iteraciones????*/
ENEMY* importEnemy(char* archivo)
{
    char linea[1024];
    int data;
    int cont = 0;
    FILE* fp = fopen(archivo, "r");
    if (fp == NULL)
    {
        printf("no se ha podido importar el archivo %s\n", archivo);
        exit(1);
    }

    ENEMY* enemy = (ENEMY*)malloc(sizeof(ENEMY));
    if (enemy == NULL)
    {
        exit(1);
    }
    enemy->Abilities = createList();
    enemy->AuraApplied = (ELEMENTAL_AURA*)malloc(sizeof(ELEMENTAL_AURA));
    if (enemy->AuraApplied == NULL)
    {
        exit(1);
    }
    enemy->objectives = (Objective_Lists*)malloc(sizeof(Objective_Lists));
    if (enemy->objectives == NULL)
    {
        exit(1);
    }
    enemy->objectives->sack = createList();

    while (fgets(linea, 1024, fp) != NULL)
    {
        if (cont == 0)
        {
            for (data = 0; data < 13; data += 1)
            {
                char* aux = (char*)get_csv_field(linea, data);
                if (data == 0)
                {
                    //ID DEL PERSONAJE
                    enemy->ID = atoi(aux);
                    printf("se ha insertado ID: %i\n", enemy->ID);
                }
                if (data == 1)
                {
                    // NIVEL
                    enemy->LVL = atoi(aux);
                    printf("se ha insertado LVL: %i\n", enemy->LVL);
                }
                if (data == 2)
                {
                    // PUNTOS DE VIDA
                    enemy->HP = atoi(aux);
                    printf("la vida actual es: %f\n", enemy->HP);
                }
                if (data == 3)
                {
                    // PUNTOS DE VIDA
                    enemy->TotalHP = atoi(aux);
                    printf("la vida máxima es: %f\n", enemy->TotalHP);
                }
                if (data == 4)
                {
                    //ATAQUE
                    enemy->ATK = atoi(aux);
                    printf("se ha insertado ATK: %f\n", enemy->ATK);
                }
                if (data == 5)
                {
                    enemy->FinalATK = atoi(aux);
                    printf("el ataque final es: %f\n", enemy->FinalATK);
                }
                if (data == 6)
                {
                    //DEFENSA
                    enemy->DEF = atoi(aux);
                    printf("se ha insertado DEF: %f\n", enemy->DEF);
                }
                if (data == 7)
                {
                    enemy->FinalDEF = atoi(aux);
                    printf("la defensa final es: %f\n", enemy->FinalDEF);
                }
                if (data == 8)
                {
                    enemy->turns = atoi(aux);
                    printf("el personaje tiene %i acciones\n", enemy->turns);
                }
                if (data == 9)
                {
                    enemy->PosHPX = atoi(aux);
                    printf("la X de la pos vida es %i\n", enemy->PosHPX);
                }
                if (data == 10)
                {
                    enemy->PosHPY = atoi(aux);
                    printf("la Y de la pos vida es %i\n", enemy->PosHPY);
                }
                if (data == 11)
                {
                    enemy->turnATKBuffCounter = atoi(aux);
                    printf("los turnos para que la modificacion de atk se termine %i\n", enemy->turnATKBuffCounter);
                }
                if (data == 12)
                {
                    enemy->turnDEFBuffCounter = atoi(aux);
                    printf("los turnos para que la modificacion de def se termine %i\n", enemy->turnDEFBuffCounter);
                }
            }
        }
        if (cont >= 1)
        {
            /*se reserva memoria para la habilidad y el aura de esta*/
            ABILITY* abilityAux = (ABILITY*)malloc(sizeof(ABILITY));
            if (abilityAux == NULL)
            {
                exit(1);
            }

            abilityAux->aura = (ELEMENTAL_AURA*)malloc(sizeof(ELEMENTAL_AURA));
            if (abilityAux->aura == NULL)
            {
                exit(1);
            }

            for (data = 0; data < 11; data += 1)
            {
                char* aux = (char*)get_csv_field(linea, data);
                if (data == 0)
                {
                    /*ESTADO DE LA HABILIDAD*/
                    abilityAux->ID = atoi(aux);
                    printf("El ID de la habilidad es: %i\n", abilityAux->ID);
                }
                if (data == 1)
                {
                    /*ESTADO DE LA HABILIDAD*/
                    abilityAux->active = atoi(aux);
                    printf("El estado de la habilidad es: %i\n", abilityAux->active);
                }
                if (data == 2)
                {
                    /*ESTADO DE LA HABILIDAD*/
                    abilityAux->Multiplier = atoi(aux);
                    printf("El multiplicador de la habilidad es: %f\n", abilityAux->Multiplier);
                }
                if (data == 3)
                {
                    /*ESTADO DE LA HABILIDAD*/
                    abilityAux->CDTurns = atoi(aux);
                    printf("Los turnos de CD son: %i\n", abilityAux->CDTurns);
                }
                if (data == 4)
                {
                    /*ESTADO DE LA HABILIDAD*/
                    abilityAux->repetitions = atoi(aux);
                    printf("Las repeticiones que tendra la habilidad son: %i\n", abilityAux->repetitions);
                }
                if (data == 5)
                {
                    /*ESTADO DE LA HABILIDAD*/
                    abilityAux->repetitionCounter = atoi(aux);
                    printf("El contador de repeticiones esta en: %i\n", abilityAux->repetitionCounter);
                }
                if (data == 6)
                {
                    /*ESTADO DE LA HABILIDAD*/
                    abilityAux->TurnsUntilEffect = atoi(aux);
                    printf("La habilidad hara efecto cada %i turnos\n", abilityAux->TurnsUntilEffect);
                }
                if (data == 7)
                {
                    /*ESTADO DE LA HABILIDAD*/
                    abilityAux->turnCounter = atoi(aux);
                    printf("El contador de turnos de la habilidad esta en: %i\n", abilityAux->turnCounter);
                }
                if (data == 8)
                {
                    abilityAux->aura->ID = atoi(aux);     //se podría usar el ID del elemento del personaje pero que paja
                    printf("el ID elemental de la habilidad es: %i\n", abilityAux->aura->ID);
                }
                if (data == 9)
                {
                    abilityAux->aura->U = atoi(aux);
                    printf("los U de la habilidad es: %f\n", abilityAux->aura->U);
                }
                if (data == 10)
                {
                    abilityAux->aura->GU = atoi(aux);
                    printf("los GU de la habilidad es: %f\n", abilityAux->aura->GU);
                }
            }

            pushFront(enemy->Abilities, abilityAux);
        }
        cont += 1;
    }
    enemy->AuraApplied->GU = 0;
    enemy->AuraApplied->U = 0;
    enemy->AuraApplied->ID = 0;

    return enemy;
}



/***************************************     COMBATE     ************************************************************/
void fillObjectivesList(List* characterList, Objective_Lists* objectives)
{
    //printf("entra");
    CHARACTER* character = firstList(characterList);
    objectives->quantitySack = 0;
    while (character != NULL)
    {
        //decidir aleatoriamente si va al principio o al final de la lista
        if (character->CurrentHP <= 0)
        {
            character = nextList(characterList);
            continue;
        }


        if (rand() % 2)
        {
            pushBack(objectives->sack, character);
            printf("el personaje %i se ingresa al final de la lista\n", character->ID);
        }
        else
        {
            pushFront(objectives->sack, character);
            printf("el personaje %i se ingresa al frente de la lista\n", character->ID);
        }

        objectives->quantitySack += 1;
        character = nextList(characterList);
    }
    //escoger numero entre 1-cantPersonajes entonces iteras X-1 para meter el personaje
}

float Base_Damage(float Talent, float ATK)
{
    return (Talent * 0.01) * ATK;
}

float HP_Scaling(float Talent, float HP)
{
    return (Talent * 0.01) * HP;
}

float Enemy_Defense_Multiplier(int CharacterLevel, int EnemyLevel, float DefenseReduction)
{
    float EnemyDefMult;
    EnemyDefMult = ((CharacterLevel + 100) / ((CharacterLevel + 100) + (EnemyLevel + 100) * (1 - DefenseReduction)));
    return EnemyDefMult;
}

void Aura_Application(ELEMENTAL_AURA* EnemyAura, ELEMENTAL_AURA* AbilityAura)
{
    EnemyAura->ID = AbilityAura->ID;
    EnemyAura->U = AbilityAura->U;      //es igual al de la habilidad para el decay,
    EnemyAura->GU = AbilityAura->GU * AURA_TAX;
}

//PODRÍA FACTORIZAR TODAS ESTAS FUNCIONES EN UNA... CREO
void Weak_Amping_Trigger(ELEMENTAL_AURA* EnemyAura, ELEMENTAL_AURA* AbilityAura)
{
    EnemyAura->GU -= AbilityAura->GU * WEAK_AMP_MODIFIER;
    if (EnemyAura->GU <= 0)
    {
        EnemyAura->ID = NO_AURA;
        EnemyAura->U = 0;
        EnemyAura->GU = 0;
    }
}

void Strong_Amping_Trigger(ELEMENTAL_AURA* EnemyAura, ELEMENTAL_AURA* AbilityAura)
{
    EnemyAura->GU -= AbilityAura->GU * STRONG_AMP_MODIFIER;
    if (EnemyAura->GU <= 0)
    {
        EnemyAura->ID = NO_AURA;
        EnemyAura->U = 0;
        EnemyAura->GU = 0;
    }
}

void Swirl_Trigger(ELEMENTAL_AURA* EnemyAura, ELEMENTAL_AURA* AbilityAura)
{
    EnemyAura->GU -= AbilityAura->GU * SWIRL_MODIFIER;
    if (EnemyAura->GU <= 0)
    {
        EnemyAura->ID = NO_AURA;
        EnemyAura->U = 0;
        EnemyAura->GU = 0;
    }
}

void ElectroCharged_Trigger(ELEMENTAL_AURA* EnemyAura, ELEMENTAL_AURA* AbilityAura)
{
    EnemyAura->GU -= ELECTROCHARGED_MODIFIER;
    if (EnemyAura->GU <= 0)
    {
        EnemyAura->ID = NO_AURA;
        EnemyAura->U = 0;
        EnemyAura->GU = 0;
    }
}

void Overload_Trigger(ELEMENTAL_AURA* EnemyAura, ELEMENTAL_AURA* AbilityAura)
{
    EnemyAura->GU -= AbilityAura->GU * OVERLOAD_MODIFIER;
    if (EnemyAura->GU <= 0)
    {
        EnemyAura->ID = NO_AURA;
        EnemyAura->U = 0;
        EnemyAura->GU = 0;
    }
}

/* Notas:
 * 1.- ReactionBonus removido porque que paja
 *
*/
float Amplifying_Reaction(ELEMENTAL_AURA* EnemyAura, ELEMENTAL_AURA* AbilityAura, int EM)
{
    float AMPReact = 1;
    int pyro = PYRO;
    int cryo = CRYO;
    int hydro = HYDRO;

    //Si el enemigo no tiene ningun aura aplicada, no hay reaccion y se aplica el elemento de la habilidad sobre el enemigo
    if (EnemyAura->ID == 0)
    {
        Aura_Application(EnemyAura, AbilityAura);
        return AMPReact;
    }

    //Para cuando los elementos sean iguales, puede que queramos ignorar esto
    /*
    if (EnemyAura->ID == AbilityAura->ID)
    {

    }
    */

    if ((AbilityAura->ID == pyro && EnemyAura->ID == cryo) || (AbilityAura->ID == hydro && EnemyAura->ID == pyro))
    {
        printf("Strong Amplifying reaction triggered!\n");
        Strong_Amping_Trigger(EnemyAura, AbilityAura);
        AMPReact = STRONG_AMP_MULTIPLIER * (1 + ((2.78 * EM) / (1400 + EM)));
    }
    else if ((AbilityAura->ID == cryo && EnemyAura->ID == pyro) || (AbilityAura->ID == pyro && EnemyAura->ID == hydro))
    {
        printf("Weak Amplifying reaction triggered!\n");
        Weak_Amping_Trigger(EnemyAura, AbilityAura);
        AMPReact = WEAK_AMP_MULTIPLIER * (1 + ((2.78 * EM) / (1400 + EM)));
    }

    return AMPReact;
}

float Transformative_Reaction(ENEMY* enemy, ELEMENTAL_AURA* AbilityAura, int EM)
{
    float proc = 0;
    float reactionBonus = 0;

    //Si el enemigo no tiene ningun aura aplicada, no hay reaccion y se aplica el elemento de la habilidad sobre el enemigo
    if (enemy->AuraApplied->ID == 0)
    {
        Aura_Application(enemy->AuraApplied, AbilityAura);
        return proc;
    }

    if (enemy->AuraApplied->ID == PYRO && AbilityAura->ID == ELECTRO)
    {
        Overload_Trigger(enemy->AuraApplied, AbilityAura);
        reactionBonus = OVERLOAD_MULTIPLIER;
    }
    if (enemy->AuraApplied->ID == HYDRO && AbilityAura->ID == ELECTRO)
    {
        ElectroCharged_Trigger(enemy->AuraApplied, AbilityAura);
        reactionBonus = ELECTROCHARGED_MULTIPLIER;
    }
    if (AbilityAura->ID == ANEMO)
    {
        //hay que hacer los ifs para los otros elementos, aunque puede que no quiera al final
        Swirl_Trigger(enemy->AuraApplied, AbilityAura);
        reactionBonus = SWIRL_MULTIPLIER;
        if (enemy->AuraApplied->ID == HYDRO)
        {
            printf("EL ENEMIGO HA SIDO DEBUFFADO\n");
            enemy->FinalATK = enemy->ATK * DEBUFF_MULTIPLIER;
            enemy->turnATKBuffCounter = 3;
        }
        if (enemy->AuraApplied->ID == PYRO)
        {
            printf("EL ENEMIGO HA SIDO BUFFADO QUE, ERES TONTO?\n");
            enemy->FinalATK = enemy->ATK * BUFF_MULTIPLIER;
            enemy->turnATKBuffCounter = 3;
        }
    }

    proc = reactionBonus * (1 + ((16 * EM) / (2000 + EM)) + reactionBonus) * ((0.00194 * 1) - (0.319 * 1) + (30.7 * 1) - 868);
    return proc;
}

/* Notas:
 * 1.- No hay crítico porque que paja
 * 2.- Se removió EnemyResistanceMultiplier porque que paja
 * 3.- Se remueve flatDMG porque que paja
 * 4.- Se remueve AMPReact porque se calculará dentro de la función
 * 5.- Se remueve TRANReac porque se calculará dentro de la función
 * 6.- Se remueve Proc porque que paja, ya veré como lo hacemos para eso, supongo que chequear si el enemigo
 *     está imbuido de cierta aura y calcular eso, hay que revisar como haremos lo de las procs de electro
 *     carga, porque joder vaya quebradero de cabeza será eso
*/
/*RETORNA UN NUMERO, NO HACE FALTA EDITAR
*/
float FinalDamage(float BaseDamage, float DMGBonus, float EnemyDEFMult, ENEMY* enemy, ELEMENTAL_AURA* AbilityAura, int EM)
{
    //DMG = ((BaseDamage + flatDMG)*(1+DMGBonus)*Crit*EnemyDEFMult*EnemyResMult*AMPReact)+TRANReac+Proc;
    printf("BaseDamage: %f\n", BaseDamage);
    printf("DMG bonus: %f\n", DMGBonus);
    printf("EnemyDEFMul: %f\n", EnemyDEFMult);

    float AMPReact = Amplifying_Reaction(enemy->AuraApplied, AbilityAura, EM);
    float TRANReact = Transformative_Reaction(enemy, AbilityAura, EM);

    float DMG;
    DMG = ((BaseDamage) * (1 + DMGBonus) * EnemyDEFMult * AMPReact) + TRANReact;
    printf("final damage: %f\n", DMG);
    return DMG;
}

/* Empieza a mirar personaje a personaje, habilidad a habilidad
 * Selecciona un personaje, selecciona una habilidad, y mira si está activa,
 * si lo esta, le sube uno el contador de repeticiones, si este es igual a las
 * repeticiones maximas que puede tener, la habilidad se termina
 *
 * HAY QUE HACER QUE CHECKEE EL ID DE LA HABILIDAD PARA QUE SE HAGA EL EFECTO QUE CORRESPONDA
 *
*/
/*SOLO ACTUALIZA HABILIDADES, NO HACE FALTA CAMBIAR NADA POR EL MOMENTO
*/
void Update_Abilities(List* characterList, List* actionList, int* cantAcciones, ENEMY* enemy)
{
    printf("\n\nRevisando habilidades:\n");
    if (firstList(actionList) == NULL)
    {
        return;
    }
    ACTION* actualAction = firstList(actionList);
    //ACTION* actualAction = lastList(actionList);
    printf("la primera accion es de %s\n", actualAction->character->name);
    printf("hay %i acciones por revisar\n", *cantAcciones);

    for (int cont = 0; cont < *cantAcciones; cont++)
    {
        printf("revisando accion de: %s\n", actualAction->character->name);

        printf("%s tiene una habilidad activa\n", actualAction->character->name);
        actualAction->abilityUsed->turnCounter += 1;
        if (actualAction->abilityUsed->turnCounter == actualAction->abilityUsed->TurnsUntilEffect)
        {
            actualAction->abilityUsed->turnCounter = 0;
            switch (actualAction->abilityUsed->ID)
            {
            case 7000:;
                //damage check
                enemy->HP -= FinalDamage(Base_Damage(actualAction->abilityUsed->Multiplier, actualAction->character->FinalATK), 1, 1,
                                         enemy, actualAction->abilityUsed->aura, actualAction->character->EM);
                break;
            case 7010:;
                //Heal check, requires to check the whole character list and then set the list on where the selected character was
                CHARACTER* selectedCharacter = firstList(characterList);
                while (selectedCharacter != NULL)
                {
                    if (selectedCharacter == NULL)
                    {
                        selectedCharacter = firstList(characterList);
                        break;
                    }
                    printf("personaje %s se ha curado\n", selectedCharacter->name);
                    selectedCharacter->CurrentHP += HP_Scaling(actualAction->abilityUsed->Multiplier, actualAction->character->TotalHP);
                    if (selectedCharacter->CurrentHP > selectedCharacter->TotalHP)
                    {
                        selectedCharacter->CurrentHP = selectedCharacter->TotalHP;
                    }
                    selectedCharacter = nextList(characterList);
                }

                if (selectedCharacter == NULL)
                {
                    selectedCharacter = firstList(characterList);
                }
                while (selectedCharacter->selected != true)
                {
                    selectedCharacter = nextList(characterList);
                    if (selectedCharacter == NULL)
                    {
                        selectedCharacter = firstList(characterList);
                    }
                }
                break;
            case 7100:;
                //ATK Buff check, requires to check the whole character list and then set the list on where the selected character was
                /*
                CHARACTER* selectedCharacter = firstList(characterList);
                while (selectedCharacter != NULL)
                {
                    selectedCharacter = nextList(characterList);
                    if (selectedCharacter == NULL)
                    {
                        selectedCharacter = firstList(characterList);
                        break;
                    }
                    printf("personaje %s se ha curado\n", selectedCharacter->name);
                    //reemplazar esta mierda por lo de buffar ataque
                    selectedCharacter->CurrentHP += HP_Scaling(actualAction->abilityUsed->Multiplier, actualAction->character->TotalHP);
                    if (selectedCharacter->CurrentHP > selectedCharacter->TotalHP)
                    {
                        selectedCharacter->CurrentHP = selectedCharacter->TotalHP;
                    }
                }

                if (selectedCharacter == NULL)
                {
                    selectedCharacter = firstList(characterList);
                }
                while (selectedCharacter->selected != true)
                {
                    selectedCharacter = nextList(characterList);
                    if (selectedCharacter == NULL)
                    {
                        selectedCharacter = firstList(characterList);
                    }
                }
                break;*/
                break;
            case 7110:;
                //DEF Buff check, requires to check the whole character list and then set the list on where the selected character was
                break;
            case 7200:;
                //ATK Debuff check
                break;
            case 7210:;
                //DEF Debuff check
                break;
            default:;
                break;
            }

                

            //hace efecto en el enemigo si es una habilidad de daño
            /*
            enemy->HP -= FinalDamage(Base_Damage(actualAction->abilityUsed->Multiplier, actualAction->character->FinalATK), 1, 1,
                enemy->AuraApplied, actualAction->abilityUsed->aura, actualAction->character->EM);*/
            printf("Enemy Aura Data:\nID: %i\nU: %f\nGU: %f\n", enemy->AuraApplied->ID, enemy->AuraApplied->U, enemy->AuraApplied->GU);

            actualAction->abilityUsed->repetitionCounter += 1;
            printf("la habilidad se ha usado: %i/%i\n", actualAction->abilityUsed->repetitionCounter, actualAction->abilityUsed->repetitions);
            if (actualAction->abilityUsed->repetitionCounter == actualAction->abilityUsed->repetitions)
            {
                printf("se desactiva la habilidad de %s\n", actualAction->character->name);
                actualAction->abilityUsed->repetitionCounter = 0;
                actualAction->abilityUsed->active = false;
                *cantAcciones -= 1;
                popCurrent(actionList);
            }
        }

        printf("se pasa a la siguiente accion\n\n");
        actualAction = nextList(actionList);
        if (actualAction == NULL)
        {
            return;
        }
        //actualAction = prevList(actionList);
    }

    printf("\n\n");
}

/*RETORNA UN PERSONAJE, NO HACE FALTA CAMBIAR NADA
*/
CHARACTER* objectiveSelection(Objective_Lists* objectives)
{
    CHARACTER* selectedObjective = firstList(objectives->sack);
    popCurrent(objectives->sack);
    objectives->quantitySack -= 1;

    printf("se selecciona como objetivo a %i\nquedan %zd elementos en el saco\n", selectedObjective->ID, objectives->quantitySack);
    return selectedObjective;
}

/*SOLO ATACA, NO HACE FALTA CAMBIAR NADA ADEMAS DE MOSTRAR EL DAÑO POR PANTALLA, IGUAL DEBERIA RETORNAR UN NUMERO PARA QUE ESTO SE PUEDA HACER
*/
void Enemy_Action(CHARACTER* character, ENEMY* enemy)
{
    ABILITY* SelectedAbility = firstList(enemy->Abilities);
    //printf("%i's hp before attack: %f\n", character->ID, character->CurrentHP);
    character->CurrentHP -= Base_Damage(enemy->FinalATK, SelectedAbility->Multiplier);
    /*que siiiiii que ya se ha calculado y calcularlo de nuevo es mal, pero es para testear y ya esta, por el momento*/
    printf("Enemy has dealed %f damage\n", Base_Damage(enemy->FinalATK, SelectedAbility->Multiplier));
}

/*HAY QUE EDITAR ESTA FUNCIÓN PARA QUE FUNCIONE CON EL ALLEGRO, ESTA ES LA PRIORIDAD NUMERO 1
*/
//CombatStart(player, enemy, display, timer, queue);
//igual habría que pasarle una flag para ver si se ganó el combate o no
//le queremos pasar la cancion como parámetro para que no sea siempre la misma?
void CombatStart(Player* player, ENEMY* enemy, ALLEGRO_DISPLAY* display, ALLEGRO_TIMER* timer, ALLEGRO_EVENT_QUEUE* queue, ALLEGRO_BITMAP* background)
{
    //al_stop_samples();
    ALLEGRO_BITMAP* pyroInfusion = al_load_bitmap("pyro400x400.png");
    must_init(pyroInfusion, "pyroInfusion");
    ALLEGRO_BITMAP* hydroInfusion = al_load_bitmap("hydro400x400.png");
    must_init(hydroInfusion, "hydroInfusion");
    ALLEGRO_BITMAP* cryoInfusion = al_load_bitmap("cryo400x400.png");
    must_init(pyroInfusion, "pyroInfusion");
    ALLEGRO_BITMAP* electroInfusion = al_load_bitmap("electro400x400.png");
    must_init(electroInfusion, "electroInfusion");

    //IMAGENES DE LAS AURAS ELEMENTALES
    ALLEGRO_FONT* actionMenuFont = al_load_ttf_font("Cirno.ttf", 69, 1);
    ALLEGRO_FONT* abilityMenuFont = al_load_ttf_font("Cirno.ttf", 45, 1);

    //hay que mostrar fondo para las habilidades????

    //se cargan los sonidos de interaccion con el menu
    ALLEGRO_SAMPLE* reverseInteractionSound = al_load_sample("00019.wav");
    must_init(reverseInteractionSound, "back");
    ALLEGRO_SAMPLE* interactionSound = al_load_sample("00042.wav");     //hay que cambiarlo por otro porque te juro que me esta haciendo un agujero en la cabeza
    must_init(interactionSound, "interaction");
    ALLEGRO_SAMPLE* selectionSound = al_load_sample("00104.wav");
    must_init(selectionSound, "selection");
    ALLEGRO_SAMPLE* abilityMenuSFX = al_load_sample("00007.wav");
    must_init(abilityMenuSFX, "abilityMenuSFX");
    ALLEGRO_SAMPLE* nullXSound = al_load_sample("00054.wav");
    must_init(nullXSound, "nullXSound");
    ALLEGRO_SAMPLE* PassActionSFX = al_load_sample("00034.wav");
    must_init(PassActionSFX, "PassAction");

    //se cargan sfx de la pelea
    ALLEGRO_SAMPLE* attackSFX = al_load_sample("00051.wav");
    must_init(attackSFX, "attackSFX");



    //se carga la cancion que va a sonar (CAMBIAR POR GROUND COLOR YELLOW DEL SWR)
    //ALLEGRO_SAMPLE* FightTheme = al_load_sample("SWR Hakurei Shrine Theme- The Ground's Colour is Yellow.wav");
    //must_init(FightTheme, "StageTheme");
    //al_play_sample(FightTheme, 0.1, 0, 1, ALLEGRO_PLAYMODE_LOOP, NULL);

    MENU* actionMenu = (MENU*)malloc(sizeof(MENU));
    if (actionMenu == NULL)
    {
        exit(1);
    }
    actionMenu->options = importMenuData("menuCombate.csv", 3);
    actionMenu->selectedOption = 0;
    actionMenu->size = 3;

    fillObjectivesList(player->characters, enemy->objectives);           //se llena la lista de objetivos del enemigo
    List* actionList = createList();            //se crea la lista de acciones
    int cantAcciones = 0;                       //se crea un contador para las acciones y poder asegurarnos que se vean solo las acciones que hubo sin repetidos

    player->AliveCharacters = 4;
    printf("personajes vivos : %i\n", player->AliveCharacters);

    CHARACTER* selectedCharacter = firstList(player->characters);
    selectedCharacter->selected = true;
    ABILITY* selectedAbility = firstList(selectedCharacter->Abilities);
    selectedAbility->selected = true;

    printf("el primer personaje seleccionado es %s\n", selectedCharacter->name);
    printf("Player Turn!\n");

    int opcion = 0;
    int playerActionCount = 0;
    bool back = true;
    bool selected = false;
    bool abilitiesOpen = false;
    bool selectedAbilityFlag = false;           //para ejecutar una habilidad tanto la flag de selected de la habilidad como esta deben estar activas
    /*PUEDE QUE QUIERA CREAR MÁS FLAGS PARA IMAGENES QUE QUIERA QUE SE MUESTREN POR MAS TIEMPO?????
    * O PODRÍA COLOCAR UN VIDEO OWO
    * PERO EL COMBATE ESTA SEMI FUNCIONAL
    */

    bool fighting = true;
    while (fighting) //while the player is fighting
    {
        //printf("entra al while principal\n");
        //al_flip_display();                          //think of it as draw the screen
        //al_clear_to_color(al_map_rgb(0,0,0));       //every pass through the game loop the channels gets cleared to black
        al_clear_to_color(al_map_rgb(0, 0, 0));

        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);           //waits for something to happen

        //checks what event happened, by comparing the type of event that we got with the possible event that happened

        //it literally crashes the game, because I'm unable to figure a better way out
        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
        {
            exit(1);
        }


        if (event.type == ALLEGRO_EVENT_KEY_DOWN)
        {
            if (event.keyboard.keycode == ALLEGRO_KEY_DOWN)
            {
                al_play_sample(interactionSound, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                //si el menu de habilidades no esta abierto y se tiene control del menu de acciones generales
                if (abilitiesOpen == false)
                {
                    actionMenu->selectedOption += 1;
                    if (actionMenu->selectedOption >= actionMenu->size)
                    {
                        actionMenu->selectedOption -= actionMenu->size;
                    }
                }
                
                //si el menu de habilidades esta abierto
                if (abilitiesOpen == true)
                {
                    //se pasa a la siguiente habilidad
                    selectedAbility->selected = false;
                    selectedAbility = nextList(selectedCharacter->Abilities);
                    if (selectedAbility == NULL)
                    {
                        selectedAbility = firstList(selectedCharacter->Abilities);
                    }
                    selectedAbility->selected = true;
                }
            }

            if (event.keyboard.keycode == ALLEGRO_KEY_UP)
            {
                al_play_sample(interactionSound, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                //si el menu de habilidades no esta abierto y se tiene control del menu de acciones generales
                if (abilitiesOpen == false)
                {
                    actionMenu->selectedOption -= 1;
                    if (actionMenu->selectedOption < 0 && abilitiesOpen == false)
                    {
                        actionMenu->selectedOption += actionMenu->size;
                    }
                }

                //si el menu de habilidades esta abierto
                if (abilitiesOpen == true)
                {
                    //se pasa a la habilidad anterior
                    selectedAbility->selected = false;
                    selectedAbility = prevList(selectedCharacter->Abilities);
                    if (selectedAbility == NULL)
                    {
                        selectedAbility = lastList(selectedCharacter->Abilities);
                    }
                    selectedAbility->selected = true;
                }
            }

            if (event.keyboard.keycode == ALLEGRO_KEY_Z)
            {
                al_play_sample(selectionSound, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                if (abilitiesOpen == false)
                {
                    selected = true;
                }
                else
                {
                    printf("se ha activado una habilidad\n");
                    selectedAbilityFlag = true;
                }
            }

            if (event.keyboard.keycode == ALLEGRO_KEY_X)
            {
                al_play_sample(nullXSound, 2, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                if (abilitiesOpen == true)
                {
                    printf("se ha salido del menu de habilidades\n");
                    //playerActionCount -= 1;
                    if (playerActionCount <= 0)
                    {
                        playerActionCount = 0;
                    }
                    abilitiesOpen = false;
                    selectedAbilityFlag = false;
                    printf("acciones: %i/%i\n", playerActionCount, player->turns);
                }
            }
        }

        //if the timer ticks this happens
        /*ORDER IS IMPORTANT, IF NOT, THE EVENT WILL BE SHOWN ON THE NEXT FRAME*/
        if (event.type == ALLEGRO_EVENT_TIMER)
        {
            al_clear_to_color(al_map_rgb(0, 0, 0));     //makes the background black
            //printf("se muestra el fondo\n");
            al_draw_bitmap(background, 0, 0, 0);

            //muestra la vida de los personajes por pantalla
            for (int characterCounter = 0; characterCounter < 4; characterCounter++)
            {
                char vidaActual[10];
                itoa(selectedCharacter->CurrentHP, vidaActual, 10);
                char vidaMaxima[10];
                itoa(selectedCharacter->TotalHP, vidaMaxima, 10);
                
                if (selectedCharacter->selected == true)
                {
                    al_draw_text(actionMenuFont, al_map_rgb(255, 255, 0), selectedCharacter->posHPX, selectedCharacter->posHPY, 0, vidaActual);
                    al_draw_text(actionMenuFont, al_map_rgb(255, 255, 0), selectedCharacter->posHPX + 95, selectedCharacter->posHPY, 0, "/");
                    al_draw_text(actionMenuFont, al_map_rgb(255, 255, 0), selectedCharacter->posHPX + 120, selectedCharacter->posHPY, 0, vidaMaxima);
                }
                else
                {
                    al_draw_text(actionMenuFont, al_map_rgb(255, 255, 255), selectedCharacter->posHPX, selectedCharacter->posHPY, 0, vidaActual);
                    al_draw_text(actionMenuFont, al_map_rgb(255, 255, 255), selectedCharacter->posHPX + 95, selectedCharacter->posHPY, 0, "/");
                    al_draw_text(actionMenuFont, al_map_rgb(255, 255, 255), selectedCharacter->posHPX + 120, selectedCharacter->posHPY, 0, vidaMaxima);
                }

                selectedCharacter = nextList(player->characters);
                if (selectedCharacter == NULL)
                {
                    selectedCharacter = firstList(player->characters);
                }
            }



            //SE MUESTRAN LOS DATOS DEL ENEMIGO
            //para vida del enemigo
            char vidaEnemigoActual[10];
            itoa(enemy->HP, vidaEnemigoActual, 10);
            char vidaEnemigoMaxima[10];
            itoa(enemy->TotalHP, vidaEnemigoMaxima, 10);
            al_draw_text(actionMenuFont, al_map_rgb(255, 255, 255), enemy->PosHPX, enemy->PosHPY, 0, vidaEnemigoActual);
            al_draw_text(actionMenuFont, al_map_rgb(255, 255, 255), enemy->PosHPX + 150, enemy->PosHPY, 0, "/");
            al_draw_text(actionMenuFont, al_map_rgb(255, 255, 255), enemy->PosHPX + 180, enemy->PosHPY, 0, vidaEnemigoMaxima);
            //chequea si el enemigo esta imbuido con algun elemento para mostrar el elemento con el que esta imbuido
            switch (enemy->AuraApplied->ID)
            {
            case PYRO:
                al_draw_bitmap(pyroInfusion, 1000, 400, 0);
                break;
            case HYDRO:
                al_draw_bitmap(hydroInfusion, 1000, 400, 0);
                break;
            case ELECTRO:
                al_draw_bitmap(electroInfusion, 1000, 400, 0);
                break;
            case CRYO:
                al_draw_bitmap(cryoInfusion, 1000, 400, 0);
                break;
            default:
                break;
            }

            //indicar si el enemigo tiene un buffo de ataque o no
            if (enemy->turnATKBuffCounter > 0)
            {
                //al_draw_bitmap(ATKBuff, x, y, 0);
            }

            //si se abren las habilidades se mostrarán por pantalla
            if (abilitiesOpen == true)
            {
                selectedAbility = firstList(selectedCharacter->Abilities);
                for (int cont = 0; cont < selectedCharacter->AmountOfAbilities; cont++)
                {
                    if (selectedAbility->selected == true)
                    {
                        al_draw_text(abilityMenuFont, al_map_rgb(255, 255, 0), selectedAbility->posX, selectedAbility->posY, 0, selectedAbility->name);
                    }
                    else
                    {
                        al_draw_text(abilityMenuFont, al_map_rgb(255, 255, 255), selectedAbility->posX, selectedAbility->posY, 0, selectedAbility->name);
                    }

                    selectedAbility = nextList(selectedCharacter->Abilities);
                    if (selectedAbility == NULL)
                    {
                        selectedAbility = firstList(selectedCharacter->Abilities);
                    }
                }

                //como es posible que el for de antes haya jodido el current, se hace este ciclo para que el currente esté en la habilidad seleccionada
                selectedAbility = firstList(selectedCharacter->Abilities);
                while (selectedAbility->selected != true)
                {
                    selectedAbility = nextList(selectedCharacter->Abilities);
                    if (selectedAbility == NULL)
                    {
                        selectedAbility = firstList(selectedCharacter->Abilities);
                    }
                }
            }

            //aqui pondria el if si la flag de seleccion de habilidades es true
            if (selectedAbilityFlag == true)
            {
                printf("se ha activado la habilidad %s\n", selectedAbility->name);
                if (selectedAbility->active == true)
                {
                    printf("se refresca la habilidad");
                    selectedAbility->repetitionCounter = 0;
                    selectedAbility->turnCounter = 0;
                    Update_Abilities(player->characters, actionList, &cantAcciones, enemy);
                    printf("Vida restante del enemigo: %f\n", enemy->HP);
                    abilitiesOpen = false;
                    selectedAbilityFlag = false;
                    //break;
                }
                else
                {
                    selectedAbility->active = true;
                    selectedAbility->repetitionCounter = 0;
                    selectedAbility->turnCounter = 0;

                    ACTION* actionData = (ACTION*)malloc(sizeof(ACTION));
                    if (actionData == NULL)
                    {
                        exit(1);
                    }
                    actionData->character = selectedCharacter;
                    actionData->abilityUsed = selectedAbility;
                    //por algun motivo cuando se usa el pushBack no se guarda como se debería guardar
                    //o mas que eso es como que falla por algun puto motivo
                    printf("se ha puesto la habilidad %s emitida por %s al principio de la lista de acciones\n", selectedAbility->name, selectedCharacter->name);
                    pushFront(actionList, actionData);
                    //hay que estar atento a esto, porque puede que quiera comentarlo y que solo se sume si la flag de seleccion es true
                    cantAcciones += 1;
                    Update_Abilities(player->characters, actionList, &cantAcciones, enemy);
                    printf("Vida restante del enemigo: %f\n", enemy->HP);
                    abilitiesOpen = false;
                    selectedAbilityFlag = false;
                }

                playerActionCount += 1;
            }


            //mostrar el aura elemental con la que está imbuido el enemigo
            /*Si el enemigo esta imbuido de algo
            * mostrar la imagen del elemento correspondiente en las 
            * coordenadas adecuadas
            * si no esta con ninguno, no mostrar nada
            */

            //como la mierda anterior desordena el current para mostrar la vida en la pantalla,
            //hago esto para que el current vuelva a estar en el personaje seleccionado
            while (selectedCharacter->selected != true)
            {
                selectedCharacter = nextList(player->characters);
                if (selectedCharacter == NULL)
                {
                    selectedCharacter = firstList(player->characters);
                }
            }

            while (selectedCharacter->CurrentHP <= 0)
            {
                selectedCharacter = nextList(player->characters);
                if (selectedCharacter == NULL)
                {
                    selectedCharacter = firstList(player->characters);
                }
            }

            //se muestra el menu segun la opcion seleccionada
            //puede que quiera activarla si y solo si no se ha abierto el menu de habilidades
            wordData* word = lastList(actionMenu->options);
            for (int cont = 0; cont < actionMenu->size; cont += 1)
            {
                if (cont == actionMenu->selectedOption)
                {
                    al_draw_text(actionMenuFont, al_map_rgb(255, 255, 0), word->x, word->y, word->flags, word->text);
                    if (selected == true)
                    {
                        al_draw_text(actionMenuFont, al_map_rgb(255, 255, 0), word->x, word->y, word->flags, word->text);
                        selected = false;
                        switch (cont)
                        {
                        case 0:;
                            al_play_sample(attackSFX, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                            enemy->HP -= selectedCharacter->FinalATK;
                            printf("%s ha inflingido %f de daño a Gino\n", selectedCharacter->name, selectedCharacter->FinalATK);
                            printf("Vida restante del enemigo: %f\n", enemy->HP);
                            //se muestra por solo 1 frame aparentemente, hay que ver como podemos hacer para que se muestre por mas tiempo
                            al_draw_text(actionMenuFont, al_map_rgb(255, 255, 255), 1320, 200, 0, "NaN");
                            Update_Abilities(player->characters, actionList, &cantAcciones, enemy);
                            break;

                        case 1:;
                            //hay que chequear el ID del personaje y rolear para ver que sonido hace ademas del de golpear
                            abilitiesOpen = true;
                            break;

                        case 2:;
                            //pasar accion
                            if (player->AliveCharacters == 1)
                            {
                                printf("el personaje seleccionado es %s\n", selectedCharacter->name);
                                playerActionCount -= 1;
                                if (playerActionCount <= 0)
                                {
                                    playerActionCount = 0;
                                }
                            }
                            else
                            {
                                selectedCharacter->selected = false;
                                selectedCharacter = nextList(player->characters);
                                if (selectedCharacter == NULL)
                                {
                                    selectedCharacter = firstList(player->characters);
                                }

                                //si se pasa de personaje y el que toca esta muerto, revisará hasta encontrar uno que esté vivo,
                                //si y solo si, hay personajes con vida
                                while (selectedCharacter->CurrentHP <= 0 && player->AliveCharacters > 1)
                                {
                                    selectedCharacter = nextList(player->characters);
                                    if (selectedCharacter == NULL)
                                    {
                                        selectedCharacter = firstList(player->characters);
                                    }
                                }
                                printf("el personaje seleccionado es %s\n", selectedCharacter->name);
                                selectedCharacter->selected = true;
                                playerActionCount -= 1;
                                if (playerActionCount <= 0)
                                {
                                    playerActionCount = 0;
                                }
                                selectedAbility = firstList(selectedCharacter->Abilities);
                                selectedAbility->selected = true;
                            }
                            
                            al_play_sample(PassActionSFX, 5, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                            break;
                        default:
                            break;
                        }

                        //porque abrir el menu no es una accion
                        if (abilitiesOpen != true)
                        {
                            playerActionCount += 1;
                        }
                        printf("acciones: %i/%i\n", playerActionCount, player->turns);
                    }

                    word = prevList(actionMenu->options);
                    if (word == NULL)
                    {
                        break;
                    }
                    continue;
                }

                if (enemy->HP <= 0)
                {
                    printf("POS SE HA MUERTO ASFJKASDNFGJKHLSHBNDKJLFGHSKLDFGLHNSBDJHKFGSKLJHDFGJLKHSDF\n");
                    fighting = false;
                }

                //si el playerActionCount == player->turns;
                //AKA es el turno del enemigo
                if (playerActionCount == player->turns)
                {
                    printf("Enemy Turn!\n");
                    printf("--------------------------\n");
                    //Si luego quiero cambiar el update abilities para acá, habría que hacer O que la habilidad haga daño
                    //en casteo o que simplemente al acabar el turno aquí se actualize
                    
                    //si el aura del enemigo se va tras un tiempo hay que poner la actualizacion aqui


                    //como se termina el turno del jugador se actualizan los buffos del enemigo
                    if (enemy->turnATKBuffCounter > 0)
                    {
                        enemy->turnATKBuffCounter -= 1;
                        printf("se reduce el contador de buffos de ataque del enemigo, quedan %i turnos\n", enemy->turnATKBuffCounter);
                        if (enemy->turnATKBuffCounter <= 0)
                        {
                            enemy->turnATKBuffCounter = 0;
                            printf("se resetea el ataque final del enemigo\n");
                            enemy->FinalATK = enemy->ATK;
                        }
                    }
                    if (enemy->turnDEFBuffCounter > 0)
                    {
                        enemy->turnDEFBuffCounter -= 1;
                        printf("se reduce el contador de buffos de defensa del enemigo, quedan %i turnos\n", enemy->turnDEFBuffCounter);
                        if (enemy->turnDEFBuffCounter <= 0)
                        {
                            enemy->turnDEFBuffCounter = 0;
                            enemy->FinalDEF = enemy->DEF;
                        }
                    }

                    for (int Enemy_Turns = 0; Enemy_Turns <= enemy->turns; Enemy_Turns += 1)
                    {
                        //decide objective
                        CHARACTER* characterObjective = objectiveSelection(enemy->objectives);
                        if (enemy->objectives->quantitySack <= 0)
                        {
                            fillObjectivesList(player->characters, enemy->objectives);
                        }

                        //CHECK IF ENEMY HAS ANY DEBUFF ENABLED

                        Enemy_Action(characterObjective, enemy);
                        printf("character %s has %f hp left\n", characterObjective->name, characterObjective->CurrentHP);
                        if (characterObjective->CurrentHP <= 0)
                        {
                            printf("%s has died\n", characterObjective->name);
                            player->AliveCharacters -= 1;
                        }
                        printf("gino has %f hp left\n", enemy->HP);
                        printf("there are %i characters alive\n", player->AliveCharacters);
                        printf("--------------------------\n");
                        break;
                    }

                    playerActionCount = 0;

                    //Si luego quiero cambiar el update abilities para acá, habría que hacer O que la habilidad haga daño
                    //en casteo o que simplemente al acabar el turno aquí se actualize

                    //si el aura del enemigo se va tras un tiempo hay que poner la actualizacion aqui

                    //como se termina el turno del enemigo se actualizan los buffos que este tiene
                    if (enemy->turnATKBuffCounter > 0)
                    {
                        enemy->turnATKBuffCounter -= 1;
                        printf("se reduce el contador de buffos de ataque del enemigo, quedan %i turnos\n", enemy->turnATKBuffCounter);
                    }
                    if (enemy->turnDEFBuffCounter > 0)
                    {
                        enemy->turnDEFBuffCounter -= 1;
                        printf("se reduce el contador de buffos de defensa del enemigo, quedan %i turnos\n", enemy->turnDEFBuffCounter);
                    }
                }

                if (player->AliveCharacters <= 0)
                {
                    fighting = false;
                }

                al_draw_text(actionMenuFont, al_map_rgb(255, 255, 255), word->x, word->y, word->flags, word->text);
                //printf("muestra la palabra %s\n", word->text);
                word = prevList(actionMenu->options);
                if (word == NULL)
                {
                    //word = lastList(actionMenu->options);
                    break;
                }
            }
            al_flip_display();  //we draw the screen
        }
    }

    al_destroy_font(actionMenuFont);
    al_destroy_sample(reverseInteractionSound);
    al_destroy_sample(interactionSound);
    al_destroy_sample(selectionSound);
    al_destroy_sample(abilityMenuSFX);
    al_destroy_sample(nullXSound);
    al_destroy_sample(PassActionSFX);
    al_destroy_sample(attackSFX);
    //al_destroy_sample(FightTheme);
}
/***************************************************************************************************************/





/*Como las conversaciones se van a basar en imagenes, hay que pasar estas imagenes a una lista, y que
* para cada vez que se avance se cargue la siguiente imagen que esta en la lista de la siguiente forma
* Los stages siguen el siguiente formato: "Stage0X_000Y.png"
*
* Estos strings se guardaran en una lista para que se pueda hacer lo siguiente
*
* ALLEGRO_BITMAP* currentDialogue = al_load_bitmap(NextList(listaDialogos));
* must_init(StageBackground, "StageBackground");
* X: Numero de Stage
* Y: Numero de dialogo
*
* Y asi cada vez que se apriete Z
* 
* cant: es la cantidad de elementos que se va a cargar en la lista
*/
List* loadConversationList(char* archive, int cant)
{
    int campo = 0;
    char linea[1024];
    FILE* fp = fopen(archive, "r");
    List* listaArchivos = createList();             //lista en la que se guardarán los archivos

    while (fgets(linea, 1024, fp) != NULL)
    {
        for (campo = 0; campo < cant; campo += 1)
        {
            char* nombre = (char*)get_csv_field(linea, campo);
            //printf("se obtiene: %s\n", nombre);
            pushFront(listaArchivos, nombre);
            printf("se ha insertado texto al inicio de la lista: %s\n", nombre);
        }
    }
    return listaArchivos;
}



/*************** NOTA SOBRE LA PROGRAMACIÓN DE LOS STAGES *****************/
/*Algo que se podría haber cambiado para que esto funcionara mejor, es hacer una funcion general para
* los stages, de forma en que no hay que hacer 3 funciones distintas para cada stage, puede que
* con una estructura llamada stage se pueda hacer algo asi, pero por el momento, me quedare con esto
*/
/**************************************************************************/

/*La idea es que esto se base en texto y combate para el texto la idea es poner una imagen de fondo,
* las imagenes de los personajes para sus mierdas, y luego el text box junto con el texto.
* 
*/
void firstPart(Player* player, ALLEGRO_DISPLAY* display, ALLEGRO_TIMER* timer, ALLEGRO_EVENT_QUEUE* queue, bool *flag)
{
    //se importa la fuente que se usará para la parte novela visual
    //puede que de igual porque al final serán imagenes pasando una detrás de otra
    ALLEGRO_FONT* StoryFont = al_load_ttf_font("Vera.ttf", 72, 1);

    //Import stage conversations
    List* conversation = loadConversationList("Stage01_Conversations.csv" , 14);

    //Importing stage background
    ALLEGRO_BITMAP* currentDialogue = al_load_bitmap(lastList(conversation));
    must_init(currentDialogue, "StageBackground");

    //Importing stage music and effects
    ALLEGRO_SAMPLE* reverseInteractionSound = al_load_sample("00019.wav");
    must_init(reverseInteractionSound, "back");
    ALLEGRO_SAMPLE* interactionSound = al_load_sample("00042.wav");
    must_init(interactionSound, "interaction");
    ALLEGRO_SAMPLE* selectionSound = al_load_sample("00104.wav");
    must_init(selectionSound, "selection");
    ALLEGRO_SAMPLE* StageTheme = al_load_sample("2022-06-24-20-29-23.wav");
    must_init(StageTheme, "StageTheme");

    al_play_sample(StageTheme, 11, 0, 1, ALLEGRO_PLAYMODE_LOOP, NULL);

    bool advance = false;
    bool conversationRunning = true;
    while (conversationRunning)
    {
        al_clear_to_color(al_map_rgb(0, 0, 0));

        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);

        //it literally crashes the game, because I'm unable to figure a better way out
        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
        {
            exit(1);
        }

        if (event.type == ALLEGRO_EVENT_KEY_DOWN)
        {
            if (event.keyboard.keycode == ALLEGRO_KEY_Z)
            {
                al_play_sample(interactionSound, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                char* dialogueAux = prevList(conversation);
                if (dialogueAux == NULL)
                {
                    //cuando se termine la conversacion se pasa al combate
                    conversationRunning = false;
                    break;
                }
                currentDialogue = al_load_bitmap(dialogueAux);
            }
        }

        if (event.type == ALLEGRO_EVENT_TIMER)
        {
            al_clear_to_color(al_map_rgb(0, 0, 0));     //makes the background black
            al_draw_bitmap(currentDialogue, 0, 0, 0);
            al_flip_display();  //we draw the screen
        }
    }

    //hay que meter un flag que marque si se gano la pelea, si se gana, se pasa al siguiente, sino, se va al menu principal
    // SE IMPORTA EL ENEMIGO Y SE EMPIEZA EL COMBATE
    al_stop_samples();
    ALLEGRO_SAMPLE* FightTheme = al_load_sample("03 Dragon Girl [Vs Meirin].wav");
    must_init(FightTheme, "StageTheme");
    al_play_sample(FightTheme, 0.5, 0, 1, ALLEGRO_PLAYMODE_LOOP, NULL);
    ENEMY* boss = importEnemy("HongMeiling.csv");
    ALLEGRO_BITMAP* background = al_load_bitmap("Stage01_Fight01.png");
    must_init(background, "background");
    CombatStart(player, boss, display, timer, queue, background);
    if (player->AliveCharacters > 0)
    {
        *flag = true;
        CHARACTER* selectedCharacter = firstList(player->characters);
        if (selectedCharacter == NULL)
        {
            exit(1);
        }

        while (selectedCharacter != NULL)
        {
            selectedCharacter->CurrentHP = selectedCharacter->TotalHP;
            printf("%s has been healed after the battle\n", selectedCharacter->name);
            selectedCharacter = nextList(player->characters);
        }

        selectedCharacter = firstList(player->characters);
    }
    else
    {
        *flag = false;
    }
    al_destroy_sample(FightTheme);
}

void secondPart(Player* player, ALLEGRO_DISPLAY* display, ALLEGRO_TIMER* timer, ALLEGRO_EVENT_QUEUE* queue, bool *flag)
{
    //se importa la fuente que se usará para la parte novela visual
    //puede que de igual porque al final serán imagenes pasando una detrás de otra
    ALLEGRO_FONT* StoryFont = al_load_ttf_font("Vera.ttf", 72, 1);

    //Import stage conversations
    List* conversation = loadConversationList("Stage02_Conversations.csv", 7);

    //Importing stage background
    ALLEGRO_BITMAP* currentDialogue = al_load_bitmap(lastList(conversation));
    must_init(currentDialogue, "currentDialogue");

    //Importing stage music and effects
    ALLEGRO_SAMPLE* reverseInteractionSound = al_load_sample("00019.wav");
    must_init(reverseInteractionSound, "back");
    ALLEGRO_SAMPLE* interactionSound = al_load_sample("00042.wav");
    must_init(interactionSound, "interaction");
    ALLEGRO_SAMPLE* selectionSound = al_load_sample("00104.wav");
    must_init(selectionSound, "selection");
    ALLEGRO_SAMPLE* StageTheme = al_load_sample("2022-06-24-20-29-23.wav");
    must_init(StageTheme, "StageTheme");

    al_play_sample(StageTheme, 0.2, 0, 1, ALLEGRO_PLAYMODE_LOOP, NULL);

    bool advance = false;
    bool conversationRunning = true;
    while (conversationRunning)
    {
        al_clear_to_color(al_map_rgb(0, 0, 0));

        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);

        if (event.type == ALLEGRO_EVENT_KEY_DOWN)
        {
            if (event.keyboard.keycode == ALLEGRO_KEY_Z)
            {
                al_play_sample(interactionSound, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                char* dialogueAux = prevList(conversation);
                if (dialogueAux == NULL)
                {
                    //cuando se termine la conversacion se pasa al combate
                    conversationRunning = false;
                    break;
                }
                currentDialogue = al_load_bitmap(dialogueAux);
            }
        }

        if (event.type == ALLEGRO_EVENT_TIMER)
        {
            al_clear_to_color(al_map_rgb(0, 0, 0));     //makes the background black
            al_draw_bitmap(currentDialogue, 0, 0, 0);
            al_flip_display();  //we draw the screen
        }
    }

    //hay que meter un flag que marque si se gano la pelea, si se gana, se pasa al siguiente, sino, se va al menu principal
    al_stop_samples();
    ALLEGRO_SAMPLE* FightTheme = al_load_sample("06 絆を胸に [Stage 7].wav");
    must_init(FightTheme, "StageTheme");
    al_play_sample(FightTheme, 0.5, 0, 1, ALLEGRO_PLAYMODE_LOOP, NULL);

    ENEMY* boss = importEnemy("Patchy.csv");
    ALLEGRO_BITMAP* background = al_load_bitmap("Stage02_Fight01.png");
    must_init(background, "background");
    CombatStart(player, boss, display, timer, queue, background);
    if (player->AliveCharacters > 0)
    {
        *flag = true;
        CHARACTER* selectedCharacter = firstList(player->characters);
        if (selectedCharacter == NULL)
        {
            exit(1);
        }

        while (selectedCharacter != NULL)
        {
            selectedCharacter->CurrentHP = selectedCharacter->TotalHP;
            printf("%s has been healed after the battle\n", selectedCharacter->name);
            selectedCharacter = nextList(player->characters);
        }

        selectedCharacter = firstList(player->characters);
    }
    else
    {
        *flag = false;
    }
    al_destroy_sample(FightTheme);
}

void thirdPart(Player* player, ALLEGRO_DISPLAY* display, ALLEGRO_TIMER* timer, ALLEGRO_EVENT_QUEUE* queue, bool *flag)
{
    //se importa la fuente que se usará para la parte novela visual
    //puede que de igual porque al final serán imagenes pasando una detrás de otra
    ALLEGRO_FONT* StoryFont = al_load_ttf_font("Vera.ttf", 72, 1);

    //Import stage conversations
    List* conversation = loadConversationList("Stage03_Conversations.csv", 1);

    //Importing stage background
    ALLEGRO_BITMAP* currentDialogue = al_load_bitmap(lastList(conversation));
    must_init(currentDialogue, "currentDialogue");

    //Importing stage music and effects
    ALLEGRO_SAMPLE* reverseInteractionSound = al_load_sample("00019.wav");
    must_init(reverseInteractionSound, "back");
    ALLEGRO_SAMPLE* interactionSound = al_load_sample("00042.wav");
    must_init(interactionSound, "interaction");
    ALLEGRO_SAMPLE* selectionSound = al_load_sample("00104.wav");
    must_init(selectionSound, "selection");
    ALLEGRO_SAMPLE* StageTheme = al_load_sample("2022-06-24-20-29-23.wav");
    must_init(StageTheme, "StageTheme");

    al_play_sample(StageTheme, 0.2, 0, 1, ALLEGRO_PLAYMODE_LOOP, NULL);

    bool advance = false;
    bool conversationRunning = true;
    while (conversationRunning)
    {
        al_clear_to_color(al_map_rgb(0, 0, 0));

        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);

        if (event.type == ALLEGRO_EVENT_KEY_DOWN)
        {
            if (event.keyboard.keycode == ALLEGRO_KEY_Z)
            {
                al_play_sample(interactionSound, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                char* dialogueAux = prevList(conversation);
                if (dialogueAux == NULL)
                {
                    //cuando se termine la conversacion se pasa al combate
                    conversationRunning = false;
                    break;
                }
                currentDialogue = al_load_bitmap(dialogueAux);
            }
        }

        if (event.type == ALLEGRO_EVENT_TIMER)
        {
            al_clear_to_color(al_map_rgb(0, 0, 0));     //makes the background black
            al_draw_bitmap(currentDialogue, 0, 0, 0);
            al_flip_display();  //we draw the screen
        }
    }

    //hay que meter un flag que marque si se gano la pelea, si se gana, se pasa al siguiente, sino, se va al menu principal
    al_stop_samples();
    ALLEGRO_SAMPLE* FightTheme = al_load_sample("07 麗しき紅の城主 [Vs Remilia].wav");
    must_init(FightTheme, "StageTheme");
    al_play_sample(FightTheme, 0.5, 0, 1, ALLEGRO_PLAYMODE_LOOP, NULL);

    ENEMY* boss = importEnemy("Remi.csv");
    ALLEGRO_BITMAP* background = al_load_bitmap("Stage03_Fight01.png");
    must_init(background, "background");
    CombatStart(player, boss, display, timer, queue, background);
    if (player->AliveCharacters > 0)
    {
        *flag = true;
        CHARACTER* selectedCharacter = firstList(player->characters);
        if (selectedCharacter == NULL)
        {
            exit(1);
        }

        while (selectedCharacter != NULL)
        {
            selectedCharacter->CurrentHP = selectedCharacter->TotalHP;
            printf("%s has been healed after the battle\n", selectedCharacter->name);
            selectedCharacter = nextList(player->characters);
        }

        selectedCharacter = firstList(player->characters);
    }
    else
    {
        *flag = false;
    }
    al_destroy_sample(FightTheme);
}



/*igual deberia retornar algo, tipo un numero y que dependiendo del numero se vaya hacia atras o se llame a otra funcion????????????????*/
void team_selection_menu (Player* player , List* CharacterList, ALLEGRO_DISPLAY* display, 
                            ALLEGRO_TIMER* timer , ALLEGRO_EVENT_QUEUE* queue , bool *flag)
{
    ALLEGRO_FONT* MenuFont = al_load_ttf_font("Cirno.ttf", 72, 1);

    ALLEGRO_BITMAP* TeamSelectionBackground = al_load_bitmap("Main_Menu.png");
    must_init(TeamSelectionBackground, "Main Menu");

    ALLEGRO_SAMPLE* reverseInteractionSound = al_load_sample("00019.wav");
    must_init(reverseInteractionSound, "back");
    ALLEGRO_SAMPLE* interactionSound = al_load_sample("00042.wav");
    must_init(interactionSound, "interaction");
    ALLEGRO_SAMPLE* selectionSound = al_load_sample("00104.wav");
    must_init(selectionSound, "selection");

    //se pasa la info del menu a la mierda esta
    MENU* teamSelection = (MENU*)malloc(sizeof(MENU));
    if (teamSelection == NULL)
    {
        exit(1);
    }
    teamSelection->options = importMenuData("teamSelection.csv", 2);
    teamSelection->selectedOption = 0;
    teamSelection->size = 2;

    bool running = true;   //igual se lo podria pasar como parametro a la wea y que se cambie desde aca dentro???
    bool selected = false;

    while (running)
    {
        al_clear_to_color(al_map_rgb(0, 0, 0));

        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);

        if (event.type == ALLEGRO_EVENT_KEY_DOWN)
        {
            if (event.keyboard.keycode == ALLEGRO_KEY_DOWN)
            {
                al_play_sample(interactionSound, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                
                teamSelection->selectedOption += 1;
                if (teamSelection->selectedOption >= teamSelection->size)
                {
                    teamSelection->selectedOption -= teamSelection->size;
                }
            }

            if (event.keyboard.keycode == ALLEGRO_KEY_UP)
            {
                al_play_sample(interactionSound, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                
                teamSelection->selectedOption -= 1;
                if (teamSelection->selectedOption < 0)
                {
                    teamSelection->selectedOption += teamSelection->size;
                }
            }

            if (event.keyboard.keycode == ALLEGRO_KEY_Z)
            {
                al_play_sample(selectionSound, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                selected = true;
            }

            if (event.keyboard.keycode == ALLEGRO_KEY_X)
            {
                al_play_sample(reverseInteractionSound, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                //la idea es que con la X se pueda ir hacia atras, por tanto...
                return;
            }
        }

        if (event.type == ALLEGRO_EVENT_TIMER)
        {
            al_draw_bitmap(TeamSelectionBackground, -50, -50, 0);

            wordData* word = lastList(teamSelection->options);
            for (int cont = 0; cont < teamSelection->size; cont += 1)
            {
                if (cont == teamSelection->selectedOption)
                {
                    al_draw_text(MenuFont, al_map_rgb(255, 0, 0), word->x, word->y, word->flags, word->text);
                    if (selected == true)
                    {
                        al_draw_text(MenuFont, al_map_rgb(255, 255, 0), word->x, word->y, word->flags, word->text);
                        selected = false;

                        switch (cont)
                        {
                        case 0:
                            printf("se seleccionó vape\n");
                            //llamar a funcion que empieze la mierda esta junto con importar los archivos correspondientes a los personajes
                            CharacterList = Import_Team_Archive("TeamVape.csv");
                            player->characters = CharacterList;
                            player->turns = 4;
                            al_stop_samples();
                            //llamar a funcion que empieze la historia
                            firstPart(player, display, timer, queue, &*flag);

                            if (*flag == true)
                            {
                                printf("segunda parte\n");
                                secondPart(player, display, timer, queue, &*flag);
                                if (*flag == true)
                                {
                                    printf("tercera parte\n");
                                    thirdPart(player, display, timer, queue, &*flag);
                                }
                            }

                            //secondPart(player, display, timer, queue, &*flag);
                            //thirdPart(player, display, timer, queue, &*flag);
                            running = false;
                            break;
                        case 1:
                            printf("se seleccionó electro\n");
                            //llamar a funcion que empieze la mierda esta junto con importar los archivos correspondientes a los personajes
                            CharacterList = Import_Team_Archive("TeamElectro.csv");
                            player->characters = CharacterList;
                            player->turns = 4;
                            al_stop_samples();
                            //estas tienen que ser distintas luego para que tenga sentido que haya DOS PUTOS EQUIPOS SABES?
                            //puede que por temas de tiempo haya que quitar esto
                            firstPart(player, display, timer, queue, &*flag);
                            secondPart(player, display, timer, queue, &*flag);
                            thirdPart(player, display, timer, queue, &*flag);
                            running = false;
                            break;
                        }
                    }

                    word = prevList(teamSelection->options);
                    if (word == NULL)
                    {
                        break;
                    }
                    continue;
                }

                al_draw_text(MenuFont, al_map_rgb(255, 255, 255), word->x, word->y, word->flags, word->text);
                word = prevList(teamSelection->options);
                if (word == NULL)
                {
                    break;
                }
            }

            al_flip_display();
        }
    }
    al_destroy_font(MenuFont);
    al_destroy_bitmap(TeamSelectionBackground);
}


//remember to check if a null is potentially returned
int main()
{
    Player* player = (Player*)malloc(sizeof(Player));
    if (player == NULL)
    {
        exit(1);
    }
    List* CharacterList = createList();

    must_init(al_init(), "allegro");                    //sets up allegro itself
    must_init(al_install_audio(), "audio");
    must_init(al_init_acodec_addon(), "acodec");
    must_init(al_install_keyboard(), "keyboard");       //keyboard inputs
    must_init(al_init_font_addon(), "font addon");      //set font
    must_init(al_init_ttf_addon(), "recognize ttf");    //recongize ttf archives
    must_init(al_init_image_addon(), "image addon");    //

    ALLEGRO_DISPLAY* display = al_create_display(1600,900);             //pointer to display, ALLEGRO_DISPLAY represents the screen, then a display is created
    ALLEGRO_TIMER* timer = al_create_timer(1.0/60);                     //REMEMBER TO START THE TIMER
    ALLEGRO_FONT* mainMenuFont = al_load_ttf_font("Cirno.ttf", 72, 1);

    ALLEGRO_SAMPLE_INSTANCE* sampleInstance = NULL;
    al_reserve_samples(6);

    ALLEGRO_SAMPLE* mainMenuTheme = al_load_sample("Touhou 11 Main Menu Theme.wav");
    must_init(mainMenuTheme, "Main Menu Theme");
    ALLEGRO_SAMPLE* reverseInteractionSound = al_load_sample("00019.wav");
    must_init(reverseInteractionSound, "back");
    ALLEGRO_SAMPLE* interactionSound = al_load_sample("00042.wav");
    must_init(interactionSound, "interaction");
    ALLEGRO_SAMPLE* selectionSound = al_load_sample("00104.wav");
    must_init(selectionSound, "selection");

    ALLEGRO_BITMAP* mainMenuBackground = al_load_bitmap("Main_Menu.png");
    must_init(mainMenuBackground, "Main_Menu");

    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();               //is where all of the events are going to come in and queue
    must_init(queue, "queue");

    //now we want to tell it that we want some of the events to go to the queue we created
    al_register_event_source(queue, al_get_keyboard_event_source());              //we want events that come from the keyboard
    al_register_event_source(queue, al_get_display_event_source(display));        //events that come from the display
    al_register_event_source(queue, al_get_timer_event_source(timer));            //this event will be created everytime the timer ticks
    //these have to be destroyed, there is a function that clears the resources so we don't have a memory leak

    MENU* mainMenu = (MENU*)malloc(sizeof(MENU));
    if (mainMenu == NULL)
    {
        exit(1);
    }
    mainMenu->options = importMenuData("menuPrincipal.csv", 2);
    mainMenu->selectedOption = 0;
    mainMenu->size = 4;

    bool selected = false;
    bool HistoriaIniciada = false;  /*Por el momento se esta usando para revisar si la cancion del menu se tiene que volver a iniciar,
                                    checkeando si la historia se empezo ya o no*/
    bool running = true;

    float mousePosX = 0;
    float mousePosY = 0;

    //game loop
    al_play_sample(mainMenuTheme, 0.2, 0, 1, ALLEGRO_PLAYMODE_LOOP , NULL);
    al_start_timer(timer);
    while (running) //while the game is running
    {
        //al_flip_display();                          //think of it as draw the screen
        //al_clear_to_color(al_map_rgb(0,0,0));       //every pass through the game loop the channels gets cleared to black
        al_clear_to_color(al_map_rgb(0, 0, 0));

        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);           //waits for something to happen

        //checks what event happened, by comparing the type of event that we got with the possible event that happened)
        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
        {
            running = false;
        }



        if (event.type == ALLEGRO_EVENT_KEY_DOWN)
        {
            if (event.keyboard.keycode == ALLEGRO_KEY_DOWN)
            {
                al_play_sample(interactionSound, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                mainMenu->selectedOption += 1;
                if (mainMenu->selectedOption >= mainMenu->size)
                {
                    mainMenu->selectedOption -= mainMenu->size;
                }
            }

            if (event.keyboard.keycode == ALLEGRO_KEY_UP)
            {
                al_play_sample(interactionSound, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                mainMenu->selectedOption -= 1;
                if (mainMenu->selectedOption < 0)
                {
                    mainMenu->selectedOption += mainMenu->size;
                }
            }

            if (event.keyboard.keycode == ALLEGRO_KEY_Z)
            {
                al_play_sample(selectionSound, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                selected = true;
            }

            if (event.keyboard.keycode == ALLEGRO_KEY_X)
            {
                al_play_sample(reverseInteractionSound, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
                mainMenu->selectedOption = 1;
            }
        }

        //if the timer ticks this happens
        /*ORDER IS IMPORTANT, IF NOT, THE EVENT WILL BE SHOWN ON THE NEXT FRAME*/
        if (event.type == ALLEGRO_EVENT_TIMER)
        {
            al_clear_to_color(al_map_rgb(0, 0, 0));     //makes the background black
            al_draw_bitmap(mainMenuBackground, -50, -50, 0);

            /*se muestra el menu segun la opcion seleccionada*/
            wordData* word = lastList(mainMenu->options);
            for (int cont = 0; cont < mainMenu->size; cont += 1)
            {
                if (cont == mainMenu->selectedOption)
                {
                    al_draw_text(mainMenuFont, al_map_rgb(255, 0, 0), word->x, word->y, word->flags, word->text);
                    if (selected == true)
                    {
                        al_draw_text(mainMenuFont, al_map_rgb(255, 255, 0), word->x, word->y, word->flags, word->text);
                        selected = false;

                        /*si se tiene seleccionado a quit game se sale del juego*/
                        switch (cont)
                        {
                            case 0:
                                team_selection_menu(player, CharacterList,display , timer , queue , &HistoriaIniciada);
                                printf("la flag es %i\n", HistoriaIniciada);
                                if (HistoriaIniciada == true)
                                {
                                    al_stop_samples();
                                    al_play_sample(mainMenuTheme, 0.2, 0, 1, ALLEGRO_PLAYMODE_LOOP, NULL);
                                    HistoriaIniciada = false;
                                }
                                break;
                            case 1:
                                //opcion no disponible
                                running = false;
                                break;
                            default:
                                break;
                        }
                    }

                    word = prevList(mainMenu->options);
                    if (word == NULL)
                    {
                        break;
                    }
                    continue;
                }
                al_draw_text(mainMenuFont, al_map_rgb(255, 255, 255), word->x, word->y, word->flags, word->text);
                //printf("muestra la palabra %s\n", word->text);
                word = prevList(mainMenu->options);
                if (word == NULL)
                {
                    break;
                }
            }
            //printf("sale del ciclo\n");
            al_flip_display();  //we draw the screen
        }
    }

    al_uninstall_keyboard();
    al_destroy_display(display);
    al_destroy_timer(timer);
    al_destroy_font(mainMenuFont);
    al_destroy_bitmap(mainMenuBackground);
    return 0;
}