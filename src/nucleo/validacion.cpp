#include "validacion.h"

#include <algorithm>
#include <cctype>

#include "reparto.h"

namespace sobres {
namespace {

std::string recortar(const std::string& texto) {
  std::size_t inicio = 0;
  std::size_t fin = texto.size();
  while (inicio < fin && std::isspace(static_cast<unsigned char>(texto[inicio]))) {
    ++inicio;
  }
  while (fin > inicio && std::isspace(static_cast<unsigned char>(texto[fin - 1]))) {
    --fin;
  }
  return texto.substr(inicio, fin - inicio);
}

std::string enMinusculas(const std::string& texto) {
  std::string r = texto;
  std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return r;
}

bool existeSobre(const std::vector<Sobre>& sobres, Id id) {
  return std::any_of(sobres.begin(), sobres.end(),
                     [id](const Sobre& s) { return s.id == id; });
}

}  // namespace

bool esColorValido(const std::string& color) {
  if (color.size() != 7 || color[0] != '#') return false;
  for (std::size_t i = 1; i < color.size(); ++i) {
    if (!std::isxdigit(static_cast<unsigned char>(color[i]))) return false;
  }
  return true;
}

std::string validarSobre(const Sobre& s, const std::vector<Sobre>& existentes) {
  const std::string nombre = recortar(s.nombre);
  if (nombre.empty()) return "El sobre necesita un nombre.";
  if (nombre.size() > 60) return "El nombre del sobre es demasiado largo.";
  if (!esColorValido(s.color)) return "El color tiene que ser del tipo #RRGGBB.";
  for (const Sobre& otro : existentes) {
    if (otro.id == s.id) continue;
    if (enMinusculas(recortar(otro.nombre)) == enMinusculas(nombre)) {
      return "Ya existe un sobre que se llama así.";
    }
  }
  return "";
}

std::string validarCategoria(const Categoria& c,
                             const std::vector<Categoria>& existentes) {
  const std::string nombre = recortar(c.nombre);
  if (nombre.empty()) return "La categoría necesita un nombre.";
  if (nombre.size() > 60) return "El nombre de la categoría es demasiado largo.";
  if (!esColorValido(c.color)) return "El color tiene que ser del tipo #RRGGBB.";
  for (const Categoria& otra : existentes) {
    if (otra.id == c.id) continue;
    if (enMinusculas(recortar(otra.nombre)) == enMinusculas(nombre)) {
      return "Ya existe una categoría que se llama así.";
    }
  }
  return "";
}

std::string validarMovimiento(const Movimiento& m,
                              const std::vector<Sobre>& sobres) {
  if (!esFechaValida(m.fecha)) return "La fecha no es válida.";
  if (m.monto <= 0) return "El monto tiene que ser mayor que cero.";
  if (m.sobreOrigen == kSinId) return "Falta elegir el sobre.";
  if (!existeSobre(sobres, m.sobreOrigen)) {
    return "El sobre del movimiento ya no existe.";
  }
  if (m.tipo == TipoMovimiento::Traspaso) {
    if (m.sobreDestino == kSinId) return "Falta el sobre de destino.";
    if (!existeSobre(sobres, m.sobreDestino)) {
      return "El sobre de destino ya no existe.";
    }
    if (m.sobreDestino == m.sobreOrigen) {
      return "Un traspaso tiene que ir de un sobre a otro distinto.";
    }
  } else if (m.categoria == kSinId) {
    // Sin categoría el reporte mensual no sirve de nada, así que se exige en
    // entradas y gastos. Los traspasos no la llevan porque no gastan dinero,
    // solo lo mueven de un sobre a otro.
    return "Falta la categoría. Escribe una: si no existe se crea sola.";
  }
  return "";
}

std::string validarPasivo(const Pasivo& p) {
  if (recortar(p.nombre).empty()) return "El pasivo necesita un nombre.";
  if (p.saldo < 0) {
    return "El saldo de un pasivo es lo que debes, así que no puede ser "
           "negativo.";
  }
  return "";
}

std::string validarMeta(const Meta& m, const std::vector<Sobre>& sobres) {
  if (recortar(m.nombre).empty()) return "La meta necesita un nombre.";
  if (m.montoObjetivo <= 0) {
    return "El monto objetivo tiene que ser mayor que cero.";
  }
  if (!esFechaValida(m.fechaObjetivo)) return "La fecha objetivo no es válida.";
  if (m.sobreAsociado == kSinId) return "Falta elegir el sobre de la meta.";
  if (!existeSobre(sobres, m.sobreAsociado)) {
    return "El sobre asociado ya no existe.";
  }
  if (m.aportePlaneado < 0) {
    return "El aporte planeado no puede ser negativo.";
  }
  if (m.tasaAnual < -0.99 || m.tasaAnual > 10.0) {
    return "La tasa anual esperada está fuera de un rango razonable.";
  }
  return "";
}

std::string validarRecurrente(const Recurrente& r,
                              const std::vector<Sobre>& sobres) {
  if (recortar(r.nombre).empty()) return "El recurrente necesita un nombre.";
  if (r.monto <= 0) return "El monto tiene que ser mayor que cero.";
  if (!esFechaValida(r.proximaFecha)) return "La próxima fecha no es válida.";
  if (r.sobreOrigen == kSinId) return "Falta elegir el sobre.";
  if (!existeSobre(sobres, r.sobreOrigen)) return "El sobre ya no existe.";
  if (r.tipo == TipoMovimiento::Traspaso) {
    if (r.sobreDestino == kSinId) return "Falta el sobre de destino.";
    if (!existeSobre(sobres, r.sobreDestino)) {
      return "El sobre de destino ya no existe.";
    }
    if (r.sobreDestino == r.sobreOrigen) {
      return "Un traspaso tiene que ir de un sobre a otro distinto.";
    }
  } else if (r.categoria == kSinId) {
    return "Falta la categoría. Escribe una: si no existe se crea sola.";
  }
  return "";
}

std::string validarPlantillaReparto(const PlantillaReparto& p,
                                    const std::vector<Sobre>& sobres) {
  if (recortar(p.nombre).empty()) return "La plantilla necesita un nombre.";
  if (p.lineas.empty()) return "La plantilla necesita al menos una línea.";
  for (const LineaReparto& l : p.lineas) {
    if (l.sobre == kSinId) return "Hay una línea sin sobre asignado.";
    if (!existeSobre(sobres, l.sobre)) {
      return "Una de las líneas apunta a un sobre que ya no existe.";
    }
    if (l.valor <= 0) return "Cada línea tiene que tener un valor positivo.";
    if (p.sobreFuente != kSinId && l.sobre == p.sobreFuente) {
      return "El sobre fuente no puede ser también uno de los destinos.";
    }
  }
  if (p.sobreFuente != kSinId && !existeSobre(sobres, p.sobreFuente)) {
    return "El sobre fuente ya no existe.";
  }
  // Sin sobre fuente la plantilla genera entradas, y las entradas llevan
  // categoría obligatoria como cualquier otra.
  if (p.sobreFuente == kSinId && p.categoria == kSinId) {
    return "Falta la categoría con la que se van a registrar las entradas.";
  }
  if (p.modo == ModoReparto::Porcentajes) {
    const std::int64_t suma = sumaDePuntosBase(p);
    if (suma != 10000) {
      return "Los porcentajes suman " + formatearPuntosBase(suma) +
             " y deben sumar exactamente 100%.";
    }
  }
  return "";
}

}  // namespace sobres
