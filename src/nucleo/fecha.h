// Sobres — fecha.h
// Una fecha de calendario sin hora ni zona horaria. El núcleo no depende de Qt
// ni de <chrono> para esto: así las pruebas del cálculo financiero compilan y
// corren sin ninguna biblioteca externa.
#pragma once

#include <optional>
#include <string>

namespace sobres {

struct Fecha {
  int anio = 1970;
  int mes = 1;   // 1 a 12
  int dia = 1;   // 1 a 31

  friend bool operator==(const Fecha& a, const Fecha& b) {
    return a.anio == b.anio && a.mes == b.mes && a.dia == b.dia;
  }
  friend bool operator!=(const Fecha& a, const Fecha& b) { return !(a == b); }
  friend bool operator<(const Fecha& a, const Fecha& b);
  friend bool operator>(const Fecha& a, const Fecha& b) { return b < a; }
  friend bool operator<=(const Fecha& a, const Fecha& b) { return !(b < a); }
  friend bool operator>=(const Fecha& a, const Fecha& b) { return !(a < b); }
};

bool esBisiesto(int anio);
int diasDelMes(int anio, int mes);
bool esFechaValida(const Fecha& f);

// Número de días desde el 1 de enero de 1970. Sirve para restar fechas.
long long aDiasSeriales(const Fecha& f);
Fecha desdeDiasSeriales(long long dias);

long long diasEntre(const Fecha& desde, const Fecha& hasta);
Fecha sumarDias(const Fecha& f, long long dias);
// Suma meses conservando el día; si el día no existe en el mes destino se
// recorta al último día de ese mes (31 de enero + 1 mes = 28 o 29 de febrero).
Fecha sumarMeses(const Fecha& f, int meses);

Fecha primerDiaDelMes(const Fecha& f);
Fecha ultimoDiaDelMes(const Fecha& f);

// Texto en formato ISO 8601 (International Organization for Standardization,
// norma 8601), es decir "2026-09-04". Es también el formato con el que se
// guardan las fechas en la base de datos, porque ordena bien como texto.
std::string aTextoIso(const Fecha& f);
std::optional<Fecha> desdeTextoIso(const std::string& texto);

// "4 de septiembre de 2026", para la interfaz.
std::string aTextoLargo(const Fecha& f);
// "septiembre 2026", para los encabezados del reporte mensual.
std::string nombreDeMes(int mes);

}  // namespace sobres
