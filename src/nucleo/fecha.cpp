#include "fecha.h"

#include <array>
#include <cstdio>

namespace sobres {
namespace {

const std::array<const char*, 13> kNombresDeMes = {
    "",        "enero",  "febrero",   "marzo",     "abril",    "mayo",
    "junio",   "julio",  "agosto",    "septiembre","octubre",  "noviembre",
    "diciembre"};

}  // namespace

bool operator<(const Fecha& a, const Fecha& b) {
  if (a.anio != b.anio) return a.anio < b.anio;
  if (a.mes != b.mes) return a.mes < b.mes;
  return a.dia < b.dia;
}

bool esBisiesto(int anio) {
  return (anio % 4 == 0 && anio % 100 != 0) || anio % 400 == 0;
}

int diasDelMes(int anio, int mes) {
  static const int dias[13] = {0, 31, 28, 31, 30, 31, 30,
                               31, 31, 30, 31, 30, 31};
  if (mes < 1 || mes > 12) return 0;
  if (mes == 2 && esBisiesto(anio)) return 29;
  return dias[mes];
}

bool esFechaValida(const Fecha& f) {
  if (f.mes < 1 || f.mes > 12) return false;
  if (f.anio < 1 || f.anio > 9999) return false;
  return f.dia >= 1 && f.dia <= diasDelMes(f.anio, f.mes);
}

// Algoritmo de calendario civil de Howard Hinnant: convierte una fecha
// gregoriana a un conteo de días con la era empezando en marzo, lo que evita
// tablas de meses y trata febrero como el último mes del año.
long long aDiasSeriales(const Fecha& f) {
  long long y = f.anio;
  const unsigned m = static_cast<unsigned>(f.mes);
  const unsigned d = static_cast<unsigned>(f.dia);
  y -= m <= 2;
  const long long era = (y >= 0 ? y : y - 399) / 400;
  const unsigned anioDeEra = static_cast<unsigned>(y - era * 400);       // 0..399
  const unsigned diaDeAnio = (153u * (m + (m > 2 ? -3u : 9u)) + 2u) / 5u + d - 1u;
  const unsigned diaDeEra =
      anioDeEra * 365u + anioDeEra / 4u - anioDeEra / 100u + diaDeAnio;
  return era * 146097 + static_cast<long long>(diaDeEra) - 719468;
}

Fecha desdeDiasSeriales(long long dias) {
  dias += 719468;
  const long long era = (dias >= 0 ? dias : dias - 146096) / 146097;
  const unsigned diaDeEra = static_cast<unsigned>(dias - era * 146097);
  const unsigned anioDeEra =
      (diaDeEra - diaDeEra / 1460u + diaDeEra / 36524u - diaDeEra / 146096u) / 365u;
  const long long y = static_cast<long long>(anioDeEra) + era * 400;
  const unsigned diaDeAnio =
      diaDeEra - (365u * anioDeEra + anioDeEra / 4u - anioDeEra / 100u);
  const unsigned mp = (5u * diaDeAnio + 2u) / 153u;
  const unsigned d = diaDeAnio - (153u * mp + 2u) / 5u + 1u;
  const unsigned m = mp + (mp < 10u ? 3u : -9u);

  Fecha f;
  f.anio = static_cast<int>(y + (m <= 2 ? 1 : 0));
  f.mes = static_cast<int>(m);
  f.dia = static_cast<int>(d);
  return f;
}

long long diasEntre(const Fecha& desde, const Fecha& hasta) {
  return aDiasSeriales(hasta) - aDiasSeriales(desde);
}

Fecha sumarDias(const Fecha& f, long long dias) {
  return desdeDiasSeriales(aDiasSeriales(f) + dias);
}

Fecha sumarMeses(const Fecha& f, int meses) {
  long long total = static_cast<long long>(f.anio) * 12 + (f.mes - 1) + meses;
  int anio = static_cast<int>(total / 12);
  int mes = static_cast<int>(total % 12);
  if (mes < 0) {
    mes += 12;
    --anio;
  }
  ++mes;
  Fecha r;
  r.anio = anio;
  r.mes = mes;
  const int limite = diasDelMes(anio, mes);
  r.dia = f.dia > limite ? limite : f.dia;
  return r;
}

Fecha primerDiaDelMes(const Fecha& f) { return Fecha{f.anio, f.mes, 1}; }

Fecha ultimoDiaDelMes(const Fecha& f) {
  return Fecha{f.anio, f.mes, diasDelMes(f.anio, f.mes)};
}

std::string aTextoIso(const Fecha& f) {
  char buffer[16];
  std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", f.anio, f.mes, f.dia);
  return std::string(buffer);
}

std::optional<Fecha> desdeTextoIso(const std::string& texto) {
  if (texto.size() != 10 || texto[4] != '-' || texto[7] != '-') {
    return std::nullopt;
  }
  for (std::size_t i = 0; i < texto.size(); ++i) {
    if (i == 4 || i == 7) continue;
    if (texto[i] < '0' || texto[i] > '9') return std::nullopt;
  }
  Fecha f;
  f.anio = std::stoi(texto.substr(0, 4));
  f.mes = std::stoi(texto.substr(5, 2));
  f.dia = std::stoi(texto.substr(8, 2));
  if (!esFechaValida(f)) return std::nullopt;
  return f;
}

std::string nombreDeMes(int mes) {
  if (mes < 1 || mes > 12) return "";
  return kNombresDeMes[static_cast<std::size_t>(mes)];
}

std::string aTextoLargo(const Fecha& f) {
  return std::to_string(f.dia) + " de " + nombreDeMes(f.mes) + " de " +
         std::to_string(f.anio);
}

}  // namespace sobres
