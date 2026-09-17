#include "estado.h"

#include <QDate>

#include "comunes.h"

namespace sobres {

Estado::Estado(Repositorio* repositorio, QObject* padre)
    : QObject(padre), repositorio_(repositorio) {
  recargar();
}

Fecha Estado::hoy() {
  const QDate d = QDate::currentDate();
  return Fecha{d.year(), d.month(), d.day()};
}

void Estado::recargar() {
  todosLosSobres_ = repositorio_->sobres(true);
  sobres_ = repositorio_->sobres(false);
  todasLasCategorias_ = repositorio_->categorias(true);
  categorias_ = repositorio_->categorias(false);
  movimientos_ = repositorio_->movimientos();
  pasivos_ = repositorio_->pasivos();
  metas_ = repositorio_->metas();
  recurrentes_ = repositorio_->recurrentes();
  plantillas_ = repositorio_->plantillas();

  // Los saldos se calculan sobre todos los sobres, incluidos los archivados:
  // si a uno archivado le quedó dinero, ese dinero sigue en la cuenta.
  saldos_ = calcularSaldos(todosLosSobres_, movimientos_);
  resumen_ = calcularPatrimonio(todosLosSobres_, saldos_, pasivos_);

  emit cambio();
}

Centavos Estado::saldoDe(Id sobre) const {
  auto it = saldos_.find(sobre);
  return it == saldos_.end() ? 0 : it->second;
}

Id Estado::asegurarCategoria(const QString& nombre, QString* error) {
  const QString limpio = nombre.trimmed();
  if (limpio.isEmpty()) return kSinId;

  for (const Categoria& c : todasLasCategorias_) {
    if (QString::fromStdString(c.nombre).compare(limpio, Qt::CaseInsensitive) !=
        0) {
      continue;
    }
    // Si se vuelve a usar una categoría archivada, se reactiva sola en vez de
    // crear una duplicada con el mismo nombre.
    if (c.archivada) {
      Categoria reactivada = c;
      reactivada.archivada = false;
      repositorio_->guardarCategoria(reactivada, error);
    }
    return c.id;
  }

  Categoria nueva;
  nueva.nombre = limpio.toStdString();
  const auto& colores = coloresSugeridos();
  nueva.color =
      colores[todasLasCategorias_.size() % colores.size()].toStdString();
  if (!repositorio_->guardarCategoria(nueva, error)) return kSinId;
  return nueva.id;
}

QString Estado::nombreDeSobre(Id id) const {
  for (const Sobre& s : todosLosSobres_) {
    if (s.id == id) return QString::fromStdString(s.nombre);
  }
  return QString();
}

QString Estado::colorDeSobre(Id id) const {
  for (const Sobre& s : todosLosSobres_) {
    if (s.id == id) return QString::fromStdString(s.color);
  }
  return QStringLiteral("#9ca3af");
}

QString Estado::nombreDeCategoria(Id id) const {
  if (id == kSinId) return QString();
  for (const Categoria& c : todasLasCategorias_) {
    if (c.id == id) return QString::fromStdString(c.nombre);
  }
  return QString();
}

}  // namespace sobres
