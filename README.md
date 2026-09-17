# Sobres

Aplicación de finanzas personales que corre solo en tu máquina. Sin nube, sin
cuentas de usuario, sin autenticación y sin red: los datos son un archivo
SQLite (Structured Query Language Lite, una base de datos que vive en un solo
archivo) en tu disco.

La cifra central no es el saldo de la cuenta. Es el patrimonio real: la suma de
los sobres que son tuyos menos lo que debes.

## Qué resuelve

En una sola cuenta bancaria conviven dineros que no son equivalentes. Está el
tuyo, está el de terceros que solo va de paso, y está el saldo que en realidad
ya le pertenece a la tarjeta de crédito. El saldo que muestra el banco los suma
todos y por eso engaña.

Sobres modela esas tres cosas por separado:

- Un sobre de tipo *propio* es dinero tuyo y disponible.
- Un sobre de tipo *ajeno* está en la cuenta pero no es tuyo. Se dibuja con
  borde punteado y fondo apagado, y no suma al patrimonio.
- Un sobre de tipo *meta* es tuyo pero está apartado. Sí suma al patrimonio.
- Un pasivo es lo que debes. Se resta del patrimonio.

La pantalla principal muestra las dos cifras lado a lado y una línea que
explica en palabras por qué no coinciden.

El reporte mensual no se lee dentro de la aplicación: se elige un mes y sale un
archivo de Excel con una hoja de resumen y una hoja por cada sobre, con su
historial de movimientos y su saldo.

## Compilar y correr en WSL (Windows Subsystem for Linux)

Necesitas un compilador de C++17, CMake y Qt 6. En Ubuntu:

    sudo apt update
    sudo apt install build-essential cmake qt6-base-dev

No hace falta instalar SQLite aparte: Qt trae su propio controlador QSQLITE.

Desde la carpeta del proyecto:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j$(nproc)
    ./build/sobres

Para correr las pruebas:

    cd build && ctest --output-on-failure

Si compilas dentro de WSL y quieres ver la ventana, Windows 11 ya trae WSLg, así
que la aplicación abre sola. Si no aparece nada, revisa que `echo $DISPLAY`
devuelva algo.

## Dónde viven los datos

Por omisión, en la carpeta de datos de aplicación de tu usuario, en un archivo
llamado `sobres.db`. La ruta exacta aparece al pie de la pestaña Ajustes.

Para respaldar, copia ese archivo. Para empezar de cero, bórralo con la
aplicación cerrada. Para trabajar con otro archivo (por ejemplo, uno de
prueba), pásalo como primer argumento:

    ./build/sobres ~/pruebas/sobres-de-prueba.db

## Cómo está organizado

    src/nucleo/     lógica financiera y escritura del Excel, sin Qt
    src/datos/      persistencia en SQLite a través de QtSql
    src/ui/         interfaz de escritorio en Qt Widgets
    pruebas/        pruebas unitarias con un marco propio de dos archivos

El núcleo está deliberadamente aislado de Qt. Eso permite que las pruebas de la
matemática de metas, del reparto de centavos y del reporte compilen y corran con
solo un compilador de C++, y hace que la lógica que importa se pueda revisar sin
tener que entender la interfaz.

## Decisiones que vale la pena conocer

### El dinero es un entero, siempre

Todos los montos se guardan como enteros de 64 bits en centavos. Nunca se usa
punto flotante para almacenar ni para sumar. El punto flotante aparece
únicamente dentro del cálculo de la anualidad de las metas, y su resultado se
redondea de vuelta a centavos antes de salir de ahí. El formato a pesos
mexicanos ocurre solo en la capa de presentación.

### Los saldos no se guardan

El saldo de cada sobre se recalcula recorriendo los movimientos cada vez que
algo cambia. No hay un campo `saldo` en la base de datos a propósito: un saldo
acumulado puede quedar desincronizado de los movimientos que lo explican, y
entonces el número deja de ser auditable. Con el volumen de datos de unas
finanzas personales, recalcular es instantáneo.

### La proyección de meta es una anualidad con valor futuro

Dado un saldo actual, una fecha objetivo y una tasa anual esperada, la ecuación
es:

    VF = VP · (1+i)^n + A · [ ((1+i)^n − 1) / i ]

donde VP es el saldo actual, VF el monto objetivo, A el aporte por periodo, n
el número de periodos que faltan e i la tasa efectiva del periodo. De ahí se
despeja el aporte requerido:

    A = (VF − VP · (1+i)^n) · i / ((1+i)^n − 1)

Cuando la tasa es cero se usa el reparto en partes iguales, que es el límite de
esa fórmula.

La tasa del periodo se obtiene con equivalencia de tasas efectivas,
i = (1 + tasa anual)^(1/m) − 1, donde m es 52, 24 o 12 según la periodicidad.
No se divide la tasa anual entre m: dividir supondría capitalización simple y
subestimaría los intereses.

Con eso la aplicación responde tres cosas: cuánto tendrías que aportar por
periodo, a dónde llegas con el aporte que sí planeaste, y en qué fecha
alcanzarías la meta manteniendo ese ritmo. Esta última despeja n de la misma
ecuación.

### El reparto no pierde centavos

Una plantilla de reparto por porcentajes asigna primero la parte entera a cada
línea y luego entrega los centavos sobrantes por el método del residuo mayor,
de modo que la suma de las partes siempre es exactamente el monto repartido. Se
puede marcar una línea para que absorba el sobrante; si no se marca ninguna, lo
absorbe la que tenga el residuo más grande. Hay pruebas que recorren cientos de
montos verificando que no se pierda ni se invente un centavo.

### El archivo de Excel se escribe a mano, sin bibliotecas

Un archivo `.xlsx` es en realidad un ZIP con varios documentos XML adentro.
`src/nucleo/xlsx.cpp` arma ese ZIP guardando las entradas sin comprimir, que es
una variante válida del formato, e implementa la suma de verificación CRC-32
(Cyclic Redundancy Check de 32 bits) que el ZIP exige. Son unas trescientas
líneas y evitan tener que instalar una biblioteca aparte solo para exportar.

Los montos salen como números con formato de moneda, no como texto, así que se
pueden sumar y filtrar dentro de Excel. Las fechas salen como fechas reales, no
como cadenas, así que se ordenan bien. El archivo generado se verificó
abriéndolo con una biblioteca independiente para confirmar que Excel lo lee.

### El reporte va por sobre, con saldo inicial y final

Cada hoja del reporte empieza con el saldo del sobre al inicio del mes, lista
los movimientos en orden con el saldo corriendo renglón por renglón, y cierra
con el saldo final. Un traspaso aparece en las dos hojas, con su signo y
diciendo de dónde vino o a dónde fue.

El saldo de cada pasivo es el que está registrado hoy, no el que tenía al
cierre del mes: la aplicación guarda el saldo actual de cada deuda, no su
historia. La hoja de resumen lo dice ahí mismo para que nadie lo lea de más.

### La categoría se escribe, no se elige de una lista

En un gasto o una entrada la categoría es obligatoria: sin ella el reporte no
sirve de nada. El campo autocompleta con lo que ya existe buscando por
cualquier parte del nombre, y si lo que se escribió es nuevo, la categoría se
da de alta al guardar. Nadie tiene que ir a Ajustes a crear la categoría y
luego volver a capturar el gasto. Si el nombre coincide con una categoría
archivada, se reactiva en lugar de crear una duplicada.

Los traspasos no llevan categoría: no gastan dinero, solo lo mueven de un sobre
a otro.

### Los recurrentes no se aplican solos

Un movimiento recurrente es una plantilla con periodicidad. La aplicación te
dice cuántas ocurrencias están vencidas, pero nunca registra nada por su
cuenta. Al aplicarlo, el movimiento se guarda con la fecha que le tocaba, no
con la de hoy, y la próxima fecha avanza un periodo desde la programada. Así un
recurrente atrasado se pone al día sin perder ninguna ocurrencia y sin que los
gastos se muevan de mes.

### Los sobres se archivan, no se borran

Un sobre con movimientos no se puede eliminar: la aplicación propone
archivarlo. El historial se conserva y los saldos siguen cuadrando. Un sobre
archivado sale de la pantalla principal pero su dinero sigue contando en el
saldo de la cuenta, porque el dinero sigue ahí.

### La aplicación arranca vacía

No hay sobres ni categorías precargadas. La primera pantalla es un texto corto
y un botón para crear el primer sobre.

## Estado de las pruebas

105 pruebas: 90 del núcleo y 15 de la capa de datos. Cubren el formateo y la
lectura de montos, la aritmética de fechas, los saldos y el patrimonio, la
proyección de metas contra valores calculados por separado, el reparto exacto
de centavos, la recurrencia, el reporte mensual, el reporte por sobre con sus
saldos, la escritura del archivo de Excel, las reglas de validación y el ida y
vuelta contra la base de datos.
