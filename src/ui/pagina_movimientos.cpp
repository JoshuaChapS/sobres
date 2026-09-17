#include "pagina_movimientos.h"

#include <QColor>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "comunes.h"
#include "dialogos.h"

namespace sobres {
namespace {

constexpr int kColumnaId = 0;
constexpr int kColumnaFecha = 1;
constexpr int kColumnaTipo = 2;
constexpr int kColumnaSobre = 3;
constexpr int kColumnaCategoria = 4;
constexpr int kColumnaMonto = 5;
constexpr int kColumnaNota = 6;

QString describeSobres(const Estado& estado, const Movimiento& m) {
  if (m.tipo == TipoMovimiento::Traspaso) {
    return estado.nombreDeSobre(m.sobreOrigen) + QStringLiteral(" → ") +
           estado.nombreDeSobre(m.sobreDestino);
  }
  return estado.nombreDeSobre(m.sobreOrigen);
}

}  // namespace

PaginaMovimientos::PaginaMovimientos(Estado* estado, QWidget* padre)
    : QWidget(padre), estado_(estado) {
  filtroSobre_ = new QComboBox;
  filtroCategoria_ = new QComboBox;
  filtroTipo_ = new QComboBox;
  filtroTipo_->addItem(tr("Todos los tipos"), -1);
  filtroTipo_->addItem(tr("Entradas"),
                       static_cast<int>(TipoMovimiento::Entrada));
  filtroTipo_->addItem(tr("Gastos"), static_cast<int>(TipoMovimiento::Gasto));
  filtroTipo_->addItem(tr("Traspasos"),
                       static_cast<int>(TipoMovimiento::Traspaso));

  auto* nuevo = new QPushButton(tr("Registrar movimiento"));
  auto* editar = new QPushButton(tr("Editar"));
  auto* eliminar = new QPushButton(tr("Eliminar"));
  connect(nuevo, &QPushButton::clicked, this,
          &PaginaMovimientos::nuevoMovimiento);
  connect(editar, &QPushButton::clicked, this,
          &PaginaMovimientos::editarSeleccionado);
  connect(eliminar, &QPushButton::clicked, this,
          &PaginaMovimientos::eliminarSeleccionado);

  auto* filaAcciones = new QHBoxLayout;
  filaAcciones->addWidget(nuevo);
  filaAcciones->addWidget(editar);
  filaAcciones->addWidget(eliminar);
  filaAcciones->addStretch();

  auto* filaFiltros = new QHBoxLayout;
  filaFiltros->addWidget(new QLabel(tr("Sobre")));
  filaFiltros->addWidget(filtroSobre_);
  filaFiltros->addSpacing(12);
  filaFiltros->addWidget(new QLabel(tr("Categoría")));
  filaFiltros->addWidget(filtroCategoria_);
  filaFiltros->addSpacing(12);
  filaFiltros->addWidget(filtroTipo_);
  filaFiltros->addStretch();

  tabla_ = new QTableWidget(0, 7);
  tabla_->setHorizontalHeaderLabels({tr("id"), tr("Fecha"), tr("Tipo"),
                                     tr("Sobre"), tr("Categoría"), tr("Monto"),
                                     tr("Nota")});
  tabla_->setColumnHidden(kColumnaId, true);
  tabla_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  tabla_->setSelectionBehavior(QAbstractItemView::SelectRows);
  tabla_->setSelectionMode(QAbstractItemView::SingleSelection);
  tabla_->verticalHeader()->setVisible(false);
  tabla_->horizontalHeader()->setSectionResizeMode(kColumnaNota,
                                                   QHeaderView::Stretch);
  connect(tabla_, &QTableWidget::doubleClicked, this,
          &PaginaMovimientos::editarSeleccionado);

  resumen_ = new QLabel;
  resumen_->setStyleSheet(
      QStringLiteral("color: %1;").arg(QString::fromLatin1(paleta::kTintaSuave)));

  auto* raiz = new QVBoxLayout(this);
  raiz->setContentsMargins(24, 20, 24, 24);
  raiz->addLayout(filaAcciones);
  raiz->addLayout(filaFiltros);
  raiz->addWidget(tabla_);
  raiz->addWidget(resumen_);

  connect(estado_, &Estado::cambio, this, &PaginaMovimientos::refrescar);
  connect(filtroSobre_, &QComboBox::currentIndexChanged, this,
          [this]() { llenarTabla(); });
  connect(filtroCategoria_, &QComboBox::currentIndexChanged, this,
          [this]() { llenarTabla(); });
  connect(filtroTipo_, &QComboBox::currentIndexChanged, this,
          [this]() { llenarTabla(); });
  refrescar();
}

void PaginaMovimientos::filtrarPorSobre(Id sobre) {
  seleccionarId(filtroSobre_, sobre);
}

void PaginaMovimientos::refrescar() {
  {
    QSignalBlocker bloqueo(filtroSobre_);
    llenarConSobres(filtroSobre_, estado_->sobresConArchivados(), true,
                    tr("Todos los sobres"));
  }
  {
    QSignalBlocker bloqueo(filtroCategoria_);
    const Id anterior = idSeleccionado(filtroCategoria_);
    filtroCategoria_->clear();
    filtroCategoria_->addItem(tr("Todas las categorías"),
                              QVariant(static_cast<qlonglong>(kSinId)));
    for (const Categoria& c : estado_->categoriasConArchivadas()) {
      filtroCategoria_->addItem(QString::fromStdString(c.nombre),
                                QVariant(static_cast<qlonglong>(c.id)));
    }
    seleccionarId(filtroCategoria_, anterior);
  }
  llenarTabla();
}

void PaginaMovimientos::llenarTabla() {
  const Id sobreFiltro = idSeleccionado(filtroSobre_);
  const Id categoriaFiltro = idSeleccionado(filtroCategoria_);
  const int tipoFiltro = filtroTipo_->currentData().toInt();

  tabla_->setRowCount(0);
  Centavos entradas = 0;
  Centavos gastos = 0;
  int contados = 0;

  for (const Movimiento& m : estado_->movimientos()) {
    if (sobreFiltro != kSinId && m.sobreOrigen != sobreFiltro &&
        m.sobreDestino != sobreFiltro) {
      continue;
    }
    if (categoriaFiltro != kSinId && m.categoria != categoriaFiltro) continue;
    if (tipoFiltro >= 0 && static_cast<int>(m.tipo) != tipoFiltro) continue;

    const int fila = tabla_->rowCount();
    tabla_->insertRow(fila);
    tabla_->setItem(fila, kColumnaId,
                    new QTableWidgetItem(QString::number(m.id)));
    tabla_->setItem(fila, kColumnaFecha,
                    new QTableWidgetItem(aQDate(m.fecha).toString(
                        QStringLiteral("dd/MM/yyyy"))));
    tabla_->setItem(fila, kColumnaTipo,
                    new QTableWidgetItem(QString::fromStdString(
                        nombreDeTipoMovimiento(m.tipo))));
    tabla_->setItem(fila, kColumnaSobre,
                    new QTableWidgetItem(describeSobres(*estado_, m)));
    tabla_->setItem(fila, kColumnaCategoria,
                    new QTableWidgetItem(estado_->nombreDeCategoria(m.categoria)));

    const QString signo = m.tipo == TipoMovimiento::Entrada
                              ? QStringLiteral("+")
                              : (m.tipo == TipoMovimiento::Gasto
                                     ? QStringLiteral("−")
                                     : QString());
    auto* celdaMonto = new QTableWidgetItem(signo + pesos(m.monto));
    celdaMonto->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    if (m.tipo == TipoMovimiento::Gasto) {
      celdaMonto->setForeground(QColor(QString::fromLatin1(paleta::kNegativo)));
    } else if (m.tipo == TipoMovimiento::Entrada) {
      celdaMonto->setForeground(QColor(QString::fromLatin1(paleta::kPositivo)));
    }
    tabla_->setItem(fila, kColumnaMonto, celdaMonto);
    tabla_->setItem(fila, kColumnaNota,
                    new QTableWidgetItem(QString::fromStdString(m.nota)));

    ++contados;
    if (m.tipo == TipoMovimiento::Entrada) entradas += m.monto;
    if (m.tipo == TipoMovimiento::Gasto) gastos += m.monto;
  }

  tabla_->resizeColumnsToContents();
  tabla_->horizontalHeader()->setSectionResizeMode(kColumnaNota,
                                                   QHeaderView::Stretch);
  tabla_->horizontalHeader()->setSectionResizeMode(kColumnaSobre,
                                                   QHeaderView::Interactive);
  resumen_->setText(tr("%1 movimientos en la vista. Entradas: %2. Gastos: %3.")
                        .arg(contados)
                        .arg(pesos(entradas))
                        .arg(pesos(gastos)));
}

Id PaginaMovimientos::movimientoSeleccionado() const {
  const int fila = tabla_->currentRow();
  if (fila < 0) return kSinId;
  const QTableWidgetItem* celda = tabla_->item(fila, kColumnaId);
  return celda ? static_cast<Id>(celda->text().toLongLong()) : kSinId;
}

void PaginaMovimientos::nuevoMovimiento() {
  if (estado_->sobres().empty()) {
    QMessageBox::information(this, tr("Falta un sobre"),
                             tr("Crea al menos un sobre antes de registrar "
                                "movimientos."));
    return;
  }
  Movimiento nuevo;
  nuevo.fecha = Estado::hoy();
  const Id sobreFiltro = idSeleccionado(filtroSobre_);
  if (sobreFiltro != kSinId) nuevo.sobreOrigen = sobreFiltro;

  DialogoMovimiento dialogo(estado_, nuevo, this);
  if (dialogo.exec() != QDialog::Accepted) return;

  Movimiento guardado = dialogo.movimiento();
  QString error;
  if (!estado_->repositorio().guardarMovimiento(guardado, &error)) {
    QMessageBox::critical(this, tr("No se pudo guardar"), error);
    return;
  }
  estado_->recargar();
}

void PaginaMovimientos::editarSeleccionado() {
  const Id id = movimientoSeleccionado();
  if (id == kSinId) return;
  for (const Movimiento& m : estado_->movimientos()) {
    if (m.id != id) continue;
    DialogoMovimiento dialogo(estado_, m, this);
    if (dialogo.exec() != QDialog::Accepted) return;
    Movimiento guardado = dialogo.movimiento();
    QString error;
    if (!estado_->repositorio().guardarMovimiento(guardado, &error)) {
      QMessageBox::critical(this, tr("No se pudo guardar"), error);
      return;
    }
    estado_->recargar();
    return;
  }
}

void PaginaMovimientos::eliminarSeleccionado() {
  const Id id = movimientoSeleccionado();
  if (id == kSinId) return;
  const auto respuesta = QMessageBox::question(
      this, tr("Eliminar movimiento"),
      tr("¿Eliminar este movimiento? Los saldos se recalculan al instante."));
  if (respuesta != QMessageBox::Yes) return;

  QString error;
  if (!estado_->repositorio().eliminarMovimiento(id, &error)) {
    QMessageBox::critical(this, tr("No se pudo eliminar"), error);
    return;
  }
  estado_->recargar();
}

}  // namespace sobres
