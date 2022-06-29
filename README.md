IDs:
4XXX => Personaje:
4000 Xiangling
4010 Bennett
4020 Fischl
4030 Kaeya
4040 Xingqiu
4050 Sucrose

5XXX => Enemigo
5000 GINO

6XXX => Elemento
0    No aura
6000 Pyro
6010 Electro
6020 Cryo
6030 Hydro
6040 Anemo

7XXX => Tipo de efecto de la habilidad
7000 Ataque
7010 Cura: escalan de la vida máxima del personaje.
7100 Buffo ataque
7110 Buffo defensa
7200 Debuffo ataque
7210 Debuffo defensa

EFECTOS DE SWIRL
HYDRO SWIRL: debuffo ataque
PYRO SWIRL: buffo ataque


Stats: Extraídas del nivel 50+ del juego original

ATK: se le suma un numero para no tener que agregar artefactos de mierda, el numero dependera de... cosas cuando estuve revisando la 
suma de ataque de los artefactos, al menos en mi cuenta, el rango estaba alrededor de los 700-1000, asi que le puse un numero alrededor 
de este rango para saltarme meter artefactos, el numero sumado está indicado en la pestaña de ataque. Puede que haya que cambiarlo para balancear luego.

Habilidades: se aproximan de forma que queden listas para aplicar multiplicandose por 0.01, porque recordar que representan porcentajes

Personajes:

Xiangling:
ID = 4000
LVL = 1
HP = 6411
ATK = 133 + 700 = 833
DEF = 394
EM = 48
Weapon = aun no
elementID = 6000
https://genshin.honeyhunterworld.com/db/char/xiangling/?lang=EN

Bennett:
ID = 4010
LVL = 1
HP = 7309
ATK = 113 + 200 = 313
DEF = 455
EM = 0
ER% = 13.3%
Weapon = aun no
elementID = 6000
https://genshin.honeyhunterworld.com/db/char/bennett/?lang=EN

Xingqiu:
ID = 4040
LVL = 1
HP = 6027
ATK = 119 + 500 = 619
DEF = 447
EM = 0
ATK% = 12%
Weapon = aun no
elementID = 6030
https://genshin.honeyhunterworld.com/db/char/kaeya/?lang=EN

Sucrose:
ID = 4050
LVL = 1
HP = 5450
ATK = 100 + 500 = 600
DEF = 414
EM = 0
AnemoDMG% = 12%
Weapon = aun no
elementID = 6040
https://genshin.honeyhunterworld.com/db/char/sucrose/?lang=EN

Como ejecutar:
Todos los archivos y el ejecutable tienen que estar en la misma carpeta.
Para abrir el juego solo se debe abrir el .exe.

Cosas que funcionan:
- El combate funciona en su totalidad.
- Los menús funcionan en su totalidad en base a listas.
- Las "conversaciones" están hechas con listas de imágenes y funcionan en todos los casos.

Cosas que no funcionan o que pueden fallar:
- Los elementos Electro y Cryo, y las reacciones Overloaded, ElectroCharged no se pueden realizar/utilizar en la aplicación a pesar de que están en el código.
- La estadística de defensa no se utiliza en ningún momento en el cálculo del daño.
- El nivel no se utiliza en ningún momento.
- La segunda habilidad de los enemigos no se utiliza en ningún momento, desperdiciando espacio en la memoria.
- La selección de equipo solamente permite que se escoja a "Team Vaporize", si se selecciona "Team Electro" el programa crashea.
- Durante el testeo se vio que la pelea contra Remilia Scarlet (el enemigo final), se vió que podrían ocurrir cosas raras con su vida y los criterios para perder la pelea.
- Bajo ciertas circunstancias la vida de un personaje muerto aparecía con el estado de seleccionado cuando ello debería ser imposible.
- La lista de habilidades activas puede fallar bajo ciertas circunstancias.
- Cuando se aprieta el botón de selección (Z) repetidamente pueden ocurrir cosas raras con el programa.