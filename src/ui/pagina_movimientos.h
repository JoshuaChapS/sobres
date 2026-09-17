// Sobres — pagina_movimientos.h
// El historial completo, con filtros por sobre, categoría y mes.
#pragma once

#include <QComboBox>
#include <QLabel>
#include <QTableWidget>
#include <QWidget>

#include "estado.h"

namespace sobres {

class PaginaMovimientos : public QWidget {
  Q_OBJECT

 public:
  explicit PaginaMovimientos(Estado* estado, QWidget* padre = nullptr);

  // La usa la pantalla principal cuando se pide ver los movimientos de un
  // sobre concreto.
  void filtrarPorSobre(Id sobre);

 public slots:
  void refrescar();

 private slots:
  void nuevoMovimiento();
  void editarSeleccionado();
  void eliminarSeleccionado();

 private:
  void llenarTabla();
  Id movimientoSeleccionado() const;

  Estado* estado_;
  QComboBox* filtroSobre_;
  QComboBox* filtroCategoria_;
  QComboBox* filtroTipo_;
  QTableWidget* tabla_;
  QLabel* resumen_;
};

}  // namespace sobres
