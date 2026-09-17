// Sobres — dinero.h
// Todo el dinero de la aplicación vive aquí como un entero de 64 bits en
// centavos. Nunca se usa punto flotante para almacenar ni para sumar montos:
// el punto flotante solo aparece dentro de la matemática financiera de metas,
// y su resultado se redondea de vuelta a centavos antes de salir.
#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace sobres {

// Un monto de dinero, siempre en centavos. 1 peso = 100 centavos.
using Centavos = std::int64_t;

// Convierte pesos con decimales a centavos redondeando al centavo más cercano
// (los medios centavos se alejan del cero: 0.005 -> 1, -0.005 -> -1).
Centavos redondearAPesosEnCentavos(double pesos);

// Redondea una cantidad que ya está expresada en centavos pero que salió de un
// cálculo con punto flotante.
Centavos redondearCentavos(double centavos);

// Formatea para la capa de presentación: 123456 -> "$1,234.56".
// Con conSigno, los positivos llevan '+' al frente.
std::string formatearPesos(Centavos monto, bool conSimbolo = true,
                           bool conSigno = false);

// Lee un monto escrito por una persona. Acepta "$1,234.56", "1234.5", "-20",
// "1 234.56" y devuelve nada si el texto no es un monto válido.
std::optional<Centavos> leerPesos(const std::string& texto);

}  // namespace sobres
