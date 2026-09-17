#include "pagina_ajustes.h"

#include <QColor>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "comunes.h"
#include "dialogos.h"

namespace sobres {
namespace {

constexpr int kColumnaId = 0;

Id idDeFila(const QTableWidget* tabla) {
  const int fila = tabla->currentRow();
  if (fila < 0) return kSinId;
  const QTableWidgetItem* celda = tabla->item(fila, kColumnaId);
  return celda ? static_cast<Id>(celda->text().toLongLong()) : kSinId;
}

QTableWidget* tablaSimple(const QStringList& encabezados) {
  auto* tabla = new QTableWidget(0, encabezados.size());
  tabla->setHorizontalHeaderLabels(encabezados);
  tabla->setColumnHidden(kColumnaId, true);
  tabla->setEditTriggers(QAbstractItemView::NoEditTriggers);
  tabla->setSelectionBehavior(QAbstractItemView::SelectRows);
  tabla->setSelectionMode(QAbstractItemView::SingleSelection);
  tabla->verticalHeader()->setVisible(false);
  tabla->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  return tabla;
}

}  // namespace

PaginaAjustes::PaginaAjustes(Estado* estado, QWidget* padre)
    : QWidget(padre), estado_(estado) {
  // --- Categorías ----------------------------------------------------------
  tablaCategorias_ = tablaSimple({tr("id"), tr("Categoría"), tr("Color")});
  auto* nuevaCat = new QPushButton(tr("Nueva"));
  auto* editarCat = new QPushButton(tr("Editar"));
  auto* borrarCat = new QPushButton(tr("Eliminar"));
  connect(nuevaCat, &QPushButton::clicked, this, &PaginaAjustes::nuevaCategoria);
  connect(editarCat, &QPushButton::clicked, this,
          &PaginaAjustes::editarCategoria);
  connect(borrarCat, &QPushButton::clicked, this,
          &PaginaAjustes::eliminarCategoria);

  auto* accionesCat = new QHBoxLayout;
  accionesCat->addWidget(nuevaCat);
  accionesCat->addWidget(editarCat);
  accionesCat->addWidget(borrarCat);
  accionesCat->addStretch();

  auto* grupoCat = new QGroupBox(tr("Categorías"));
  auto* columnaCat = new QVBoxLayout(grupoCat);
  columnaCat->addLayout(accionesCat);
  columnaCat->addWidget(tablaCategorias_);

  // --- Pasivos -------------------------------------------------------------
  tablaPasivos_ = tablaSimple({tr("id"), tr("Pasivo"), tr("Saldo"), tr("Nota")});
  auto* nuevoPas = new QPushButton(tr("Nuevo"));
  auto* editarPas = new QPushButton(tr("Editar"));
  auto* borrarPas = new QPushButton(tr("Eliminar"));
  connect(nuevoPas, &QPushButton::clicked, this, &PaginaAjustes::nuevoPasivo);
  connect(editarPas, &QPushButton::clicked, this, &PaginaAjustes::editarPasivo);
  connect(borrarPas, &QPushButton::clicked, this,
          &PaginaAjustes::eliminarPasivo);

  auto* accionesPas = new QHBoxLayout;
  accionesPas->addWidget(nuevoPas);
  accionesPas->addWidget(editarPas);
  accionesPas->addWidget(borrarPas);
  accionesPas->addStretch();

  totalPasivos_ = new QLabel;
  totalPasivos_->setWordWrap(true);
  totalPasivos_->setStyleSheet(
      QStringLiteral("color: %1;").arg(QString::fromLatin1(paleta::kTintaSuave)));

  auto* grupoPas = new QGroupBox(tr("Pasivos"));
  auto* columnaPas = new QVBoxLayout(grupoPas);
  columnaPas->addLayout(accionesPas);
  columnaPas->addWidget(tablaPasivos_);
  columnaPas->addWidget(totalPasivos_);

  // --- Sobres archivados ---------------------------------------------------
  tablaArchivados_ = tablaSimple({tr("id"), tr("Sobre"), tr("Saldo")});
  auto* restaurar = new QPushButton(tr("Restaurar"));
  auto* borrarSobre = new QPushButton(tr("Eliminar definitivamente"));
  connect(restaurar, &QPushButton::clicked, this,
          &PaginaAjustes::restaurarSobre);
  connect(borrarSobre, &QPushButton::clicked, this,
          &PaginaAjustes::eliminarSobreArchivado);

  auto* accionesArch = new QHBoxLayout;
  accionesArch->addWidget(restaurar);
  accionesArch->addWidget(borrarSobre);
  accionesArch->addStretch();

  auto* grupoArch = new QGroupBox(tr("Sobres archivados"));
  auto* columnaArch = new QVBoxLayout(grupoArch);
  columnaArch->addLayout(accionesArch);
  columnaArch->addWidget(tablaArchivados_);

  rutaArchivo_ = new QLabel;
  rutaArchivo_->setWordWrap(true);
  rutaArchivo_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  rutaArchivo_->setStyleSheet(
      QStringLiteral("color: %1;").arg(QString::fromLatin1(paleta::kTintaSuave)));

  auto* raiz = new QVBoxLayout(this);
  raiz->setContentsMargins(24, 20, 24, 24);
  raiz->addWidget(grupoCat);
  raiz->addWidget(grupoPas);
  raiz->addWidget(grupoArch);
  raiz->addStretch();
  raiz->addWidget(rutaArchivo_);

  connect(estado_, &Estado::cambio, this, &PaginaAjustes::refrescar);
  refrescar();
}

void PaginaAjustes::refrescar() {
  tablaCategorias_->setRowCount(0);
  for (const Categoria& c : estado_->categorias()) {
    const int fila = tablaCategorias_->rowCount();
    tablaCategorias_->insertRow(fila);
    tablaCategorias_->setItem(fila, kColumnaId,
                              new QTableWidgetItem(QString::number(c.id)));
    QString nombre = QString::fromStdString(c.nombre);
    if (!c.icono.empty()) {
      nombre = QString::fromStdString(c.icono) + QStringLiteral("  ") + nombre;
    }
    tablaCategorias_->setItem(fila, 1, new QTableWidgetItem(nombre));
    auto* celdaColor = new QTableWidgetItem(QString::fromStdString(c.color));
    celdaColor->setForeground(QColor(QString::fromStdString(c.color)));
    tablaCategorias_->setItem(fila, 2, celdaColor);
  }

  tablaPasivos_->setRowCount(0);
  Centavos total = 0;
  for (const Pasivo& p : estado_->pasivos()) {
    const int fila = tablaPasivos_->rowCount();
    tablaPasivos_->insertRow(fila);
    tablaPasivos_->setItem(fila, kColumnaId,
                           new QTableWidgetItem(QString::number(p.id)));
    tablaPasivos_->setItem(
        fila, 1, new QTableWidgetItem(QString::fromStdString(p.nombre)));
    auto* celdaSaldo = new QTableWidgetItem(pesos(p.saldo));
    celdaSaldo->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    tablaPasivos_->setItem(fila, 2, celdaSaldo);
    tablaPasivos_->setItem(
        fila, 3, new QTableWidgetItem(QString::fromStdString(p.nota)));
    total += p.saldo;
  }
  totalPasivos_->setText(
      estado_->pasivos().empty()
          ? tr("Sin pasivos registrados. Si tienes tarjeta de crédito, "
               "agrégala: parte del saldo de tu cuenta ya le pertenece.")
          : tr("Total que debes: %1. Eso es lo que se resta del patrimonio "
               "real.")
                .arg(pesos(total)));

  tablaArchivados_->setRowCount(0);
  for (const Sobre& s : estado_->sobresConArchivados()) {
    if (!s.archivado) continue;
    const int fila = tablaArchivados_->rowCount();
    tablaArchivados_->insertRow(fila);
    tablaArchivados_->setItem(fila, kColumnaId,
                              new QTableWidgetItem(QString::number(s.id)));
    tablaArchivados_->setItem(
        fila, 1, new QTableWidgetItem(QString::fromStdString(s.nombre)));
    auto* celdaSaldo = new QTableWidgetItem(pesos(estado_->saldoDe(s.id)));
    celdaSaldo->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    tablaArchivados_->setItem(fila, 2, celdaSaldo);
  }

  ajustarAlturaDeTabla(tablaCategorias_, 8, 3);
  ajustarAlturaDeTabla(tablaPasivos_, 6, 2);
  ajustarAlturaDeTabla(tablaArchivados_, 6, 2);

  rutaArchivo_->setText(
      tr("Tus datos viven solo en este archivo: %1. Para respaldarlos, "
         "cópialo. Para empezar de cero, bórralo con la aplicación cerrada.")
          .arg(estado_->repositorio().rutaArchivo()));
}

Id PaginaAjustes::categoriaSeleccionada() const {
  return idDeFila(tablaCategorias_);
}
Id PaginaAjustes::pasivoSeleccionado() const { return idDeFila(tablaPasivos_); }
Id PaginaAjustes::archivadoSeleccionado() const {
  return idDeFila(tablaArchivados_);
}

void PaginaAjustes::nuevaCategoria() {
  Categoria nueva;
  nueva.color = coloresSugeridos()[estado_->categorias().size() %
                                   coloresSugeridos().size()]
                    .toStdString();
  DialogoCategoria dialogo(nueva, estado_->categoriasConArchivadas(), this);
  if (dialogo.exec() != QDialog::Accepted) return;
  Categoria guardada = dialogo.categoria();
  QString error;
  if (!estado_->repositorio().guardarCategoria(guardada, &error)) {
    QMessageBox::critical(this, tr("No se pudo guardar"), error);
    return;
  }
  estado_->recargar();
}

void PaginaAjustes::editarCategoria() {
  const Id id = categoriaSeleccionada();
  if (id == kSinId) return;
  for (const Categoria& c : estado_->categorias()) {
    if (c.id != id) continue;
    DialogoCategoria dialogo(c, estado_->categoriasConArchivadas(), this);
    if (dialogo.exec() != QDialog::Accepted) return;
    Categoria guardada = dialogo.categoria();
    QString error;
    if (!estado_->repositorio().guardarCategoria(guardada, &error)) {
      QMessageBox::critical(this, tr("No se pudo guardar"), error);
      return;
    }
    estado_->recargar();
    return;
  }
}

void PaginaAjustes::eliminarCategoria() {
  const Id id = categoriaSeleccionada();
  if (id == kSinId) return;
  const auto respuesta = QMessageBox::question(
      this, tr("Eliminar categoría"),
      tr("Los movimientos que la usaban se quedan sin categoría, no se "
         "borran. ¿Continuar?"));
  if (respuesta != QMessageBox::Yes) return;
  QString error;
  if (!estado_->repositorio().eliminarCategoria(id, &error)) {
    QMessageBox::critical(this, tr("No se pudo eliminar"), error);
    return;
  }
  estado_->recargar();
}

void PaginaAjustes::nuevoPasivo() {
  DialogoPasivo dialogo(Pasivo{}, this);
  if (dialogo.exec() != QDialog::Accepted) return;
  Pasivo guardado = dialogo.pasivo();
  QString error;
  if (!estado_->repositorio().guardarPasivo(guardado, &error)) {
    QMessageBox::critical(this, tr("No se pudo guardar"), error);
    return;
  }
  estado_->recargar();
}

void PaginaAjustes::editarPasivo() {
  const Id id = pasivoSeleccionado();
  if (id == kSinId) return;
  for (const Pasivo& p : estado_->pasivos()) {
    if (p.id != id) continue;
    DialogoPasivo dialogo(p, this);
    if (dialogo.exec() != QDialog::Accepted) return;
    Pasivo guardado = dialogo.pasivo();
    QString error;
    if (!estado_->repositorio().guardarPasivo(guardado, &error)) {
      QMessageBox::critical(this, tr("No se pudo guardar"), error);
      return;
    }
    estado_->recargar();
    return;
  }
}

void PaginaAjustes::eliminarPasivo() {
  const Id id = pasivoSeleccionado();
  if (id == kSinId) return;
  QString error;
  if (!estado_->repositorio().eliminarPasivo(id, &error)) {
    QMessageBox::critical(this, tr("No se pudo eliminar"), error);
    return;
  }
  estado_->recargar();
}

void PaginaAjustes::restaurarSobre() {
  const Id id = archivadoSeleccionado();
  if (id == kSinId) return;
  QString error;
  if (!estado_->repositorio().archivarSobre(id, false, &error)) {
    QMessageBox::critical(this, tr("No se pudo restaurar"), error);
    return;
  }
  estado_->recargar();
}

void PaginaAjustes::eliminarSobreArchivado() {
  const Id id = archivadoSeleccionado();
  if (id == kSinId) return;
  QString error;
  if (!estado_->repositorio().eliminarSobre(id, &error)) {
    QMessageBox::warning(this, tr("No se puede eliminar"), error);
    return;
  }
  estado_->recargar();
}

}  // namespace sobres
