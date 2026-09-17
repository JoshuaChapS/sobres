#include "modelos.h"

namespace sobres {

std::string nombreDeTipo(TipoSobre tipo) {
  switch (tipo) {
    case TipoSobre::Propio: return "Propio";
    case TipoSobre::Ajeno: return "De terceros";
    case TipoSobre::Meta: return "Meta";
  }
  return "Propio";
}

const char* claveDeTipo(TipoSobre tipo) {
  switch (tipo) {
    case TipoSobre::Propio: return "propio";
    case TipoSobre::Ajeno: return "ajeno";
    case TipoSobre::Meta: return "meta";
  }
  return "propio";
}

std::string nombreDeTipoMovimiento(TipoMovimiento tipo) {
  switch (tipo) {
    case TipoMovimiento::Entrada: return "Entrada";
    case TipoMovimiento::Gasto: return "Gasto";
    case TipoMovimiento::Traspaso: return "Traspaso";
  }
  return "Gasto";
}

std::string nombreDePeriodicidad(Periodicidad p) {
  switch (p) {
    case Periodicidad::Semanal: return "Semanal";
    case Periodicidad::Quincenal: return "Quincenal";
    case Periodicidad::Mensual: return "Mensual";
  }
  return "Mensual";
}

int periodosPorAnio(Periodicidad p) {
  switch (p) {
    case Periodicidad::Semanal: return 52;
    case Periodicidad::Quincenal: return 24;
    case Periodicidad::Mensual: return 12;
  }
  return 12;
}

}  // namespace sobres
