// Sobres — pagina_automatizaciones.h
// Los movimientos recurrentes y las plantillas de reparto. Nada de esto ocurre
// solo: los recurrentes se materializan con un botón y los repartos se aplican
// cuando el usuario lo decide.
#pragma once

#include <QLabel>
#include <QTableWidget>
#include <QWidget>

#include "estado.h"

namespace sobres {

class PaginaAutomatizaciones : public QWidget {
  Q_OBJECT

 public:
  explicit PaginaAutomatizaciones(Estado* estado, QWidget* padre = nullptr);

 public slots:
  void refrescar();

 private slots:
  void nuevoRecurrente();
  void editarRecurrente();
  void eliminarRecurrente();
  void materializarRecurrente();

  void nuevaPlantilla();
  void editarPlantilla();
  void eliminarPlantilla();
  void aplicarPlantilla();

 private:
  Id recurrenteSeleccionado() const;
  Id plantillaSeleccionada() const;
  void llenarRecurrentes();
  void llenarPlantillas();

  Estado* estado_;
  QTableWidget* tablaRecurrentes_;
  QTableWidget* tablaPlantillas_;
  QLabel* avisoPendientes_;
};

}  // namespace sobres
