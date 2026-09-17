// Sobres — pagina_ajustes.h
// Categorías, pasivos, sobres archivados y dónde vive el archivo de datos.
#pragma once

#include <QLabel>
#include <QTableWidget>
#include <QWidget>

#include "estado.h"

namespace sobres {

class PaginaAjustes : public QWidget {
  Q_OBJECT

 public:
  explicit PaginaAjustes(Estado* estado, QWidget* padre = nullptr);

 public slots:
  void refrescar();

 private slots:
  void nuevaCategoria();
  void editarCategoria();
  void eliminarCategoria();

  void nuevoPasivo();
  void editarPasivo();
  void eliminarPasivo();

  void restaurarSobre();
  void eliminarSobreArchivado();

 private:
  Id categoriaSeleccionada() const;
  Id pasivoSeleccionado() const;
  Id archivadoSeleccionado() const;

  Estado* estado_;
  QTableWidget* tablaCategorias_;
  QTableWidget* tablaPasivos_;
  QTableWidget* tablaArchivados_;
  QLabel* totalPasivos_;
  QLabel* rutaArchivo_;
};

}  // namespace sobres
