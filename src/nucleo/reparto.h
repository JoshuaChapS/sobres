// Sobres — reparto.h
// Aplicar una distribución fija (el sueldo de la quincena repartido en varios
// sobres) en una sola acción.
//
// El reparto por porcentajes reparte hasta el último centavo: se asigna la
// parte entera a cada línea y los centavos que sobran por el redondeo se
// entregan por el método del residuo mayor, de modo que la suma de las partes
// siempre es exactamente el monto repartido.
#pragma once

#include <string>
#include <vector>

#include "modelos.h"

namespace sobres {

struct AsignacionReparto {
  Id sobre = kSinId;
  Centavos monto = 0;
};

struct ResultadoReparto {
  bool valido = false;
  std::string problema;
  std::vector<AsignacionReparto> asignaciones;
  Centavos total = 0;  // suma de las asignaciones
};

// En modo MontosFijos el parámetro montoTotal se ignora y el total sale de la
// suma de las líneas. En modo Porcentajes se reparte montoTotal.
ResultadoReparto calcularReparto(const PlantillaReparto& plantilla,
                                 Centavos montoTotal);

// Convierte el resultado en movimientos listos para guardar: entradas si la
// plantilla no tiene sobre fuente, traspasos si lo tiene.
std::vector<Movimiento> generarMovimientosDeReparto(
    const PlantillaReparto& plantilla, const ResultadoReparto& resultado,
    const Fecha& fecha);

// Suma de los porcentajes en puntos base. Debe dar 10000 para que la plantilla
// sea aplicable.
std::int64_t sumaDePuntosBase(const PlantillaReparto& plantilla);

// Formatea puntos base como porcentaje: 2550 -> "25.5%".
std::string formatearPuntosBase(std::int64_t puntosBase);

}  // namespace sobres
