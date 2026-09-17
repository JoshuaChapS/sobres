#include "dinero.h"

#include <cmath>
#include <cstdlib>
#include <sstream>

namespace sobres {

Centavos redondearCentavos(double centavos) {
  if (!std::isfinite(centavos)) return 0;
  // std::llround aleja del cero los medios, que es lo que espera cualquiera
  // que revise una cuenta a mano.
  return static_cast<Centavos>(std::llround(centavos));
}

Centavos redondearAPesosEnCentavos(double pesos) {
  return redondearCentavos(pesos * 100.0);
}

std::string formatearPesos(Centavos monto, bool conSimbolo, bool conSigno) {
  const bool negativo = monto < 0;
  // Se usa unsigned para que el mínimo de int64 no desborde al negarlo.
  std::uint64_t absoluto =
      negativo ? (~static_cast<std::uint64_t>(monto) + 1u)
               : static_cast<std::uint64_t>(monto);

  const std::uint64_t enteros = absoluto / 100u;
  const std::uint64_t decimales = absoluto % 100u;

  std::string parteEntera = std::to_string(enteros);
  std::string conComas;
  int cuenta = 0;
  for (auto it = parteEntera.rbegin(); it != parteEntera.rend(); ++it) {
    if (cuenta == 3) {
      conComas.push_back(',');
      cuenta = 0;
    }
    conComas.push_back(*it);
    ++cuenta;
  }
  std::string salida(conComas.rbegin(), conComas.rend());

  std::ostringstream flujo;
  if (negativo) {
    flujo << '-';
  } else if (conSigno && monto > 0) {
    flujo << '+';
  }
  if (conSimbolo) flujo << '$';
  flujo << salida << '.';
  if (decimales < 10) flujo << '0';
  flujo << decimales;
  return flujo.str();
}

std::optional<Centavos> leerPesos(const std::string& texto) {
  std::string limpio;
  limpio.reserve(texto.size());
  bool negativo = false;
  bool vioSigno = false;
  bool vioDigito = false;
  bool vioPunto = false;
  int decimalesLeidos = 0;

  for (char c : texto) {
    if (c == ' ' || c == '\t' || c == ',' || c == '$') continue;
    if (c == '+' || c == '-') {
      // Un signo solo es válido antes de cualquier dígito y solo una vez.
      if (vioSigno || vioDigito || vioPunto) return std::nullopt;
      vioSigno = true;
      negativo = (c == '-');
      continue;
    }
    if (c == '.') {
      if (vioPunto) return std::nullopt;
      vioPunto = true;
      continue;
    }
    if (c < '0' || c > '9') return std::nullopt;
    vioDigito = true;
    if (vioPunto) {
      ++decimalesLeidos;
      if (decimalesLeidos > 2) return std::nullopt;  // no hay medios centavos
    }
    limpio.push_back(c);
  }

  if (!vioDigito) return std::nullopt;
  // "12.5" son 12 pesos con 50 centavos, no con 5.
  while (decimalesLeidos < 2) {
    limpio.push_back('0');
    ++decimalesLeidos;
  }

  Centavos acumulado = 0;
  for (char c : limpio) {
    const int digito = c - '0';
    // Corta antes de desbordar en lugar de dar un número basura.
    if (acumulado > (INT64_MAX - digito) / 10) return std::nullopt;
    acumulado = acumulado * 10 + digito;
  }
  return negativo ? -acumulado : acumulado;
}

}  // namespace sobres
