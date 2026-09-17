// Sobres — pagina_metas.h
// Cada meta con su proyección: cuánto habría que aportar, cómo se compara con
// lo planeado y cuándo se alcanzaría al ritmo actual.
#pragma once

#include <QVBoxLayout>
#include <QWidget>

#include "estado.h"

namespace sobres {

class PaginaMetas : public QWidget {
  Q_OBJECT

 public:
  explicit PaginaMetas(Estado* estado, QWidget* padre = nullptr);

 public slots:
  void refrescar();

 private slots:
  void nuevaMeta();

 private:
  QWidget* construirTarjeta(const Meta& meta);

  Estado* estado_;
  QVBoxLayout* lista_;
  QWidget* aviso_;
};

}  // namespace sobres
