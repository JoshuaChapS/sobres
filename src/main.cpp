// Sobres — main.cpp
// Arranque de la aplicación. Todo corre en esta máquina: no hay red, no hay
// cuentas y no hay inicio de sesión. Los datos son un archivo SQLite
// (Structured Query Language Lite) en el disco del usuario.
//
// La ruta del archivo se puede cambiar con el primer argumento de la línea de
// comandos, lo que sirve para tener un perfil aparte o para probar sin tocar
// los datos de verdad.
#include <QApplication>
#include <QLocale>
#include <QMessageBox>

#include "datos/repositorio.h"
#include "ui/comunes.h"
#include "ui/ventana_principal.h"

namespace {

QString hojaDeEstiloGlobal() {
  using namespace sobres::paleta;
  return QStringLiteral(R"(
    QWidget { color: %1; font-size: 10pt; }
    QMainWindow, QTabWidget::pane, QScrollArea { background-color: %2; }
    QTabWidget::pane { border: none; border-top: 1px solid %3; }
    QTabBar::tab {
      background: transparent; padding: 8px 16px; margin-right: 2px;
      color: %4; border-bottom: 2px solid transparent;
    }
    QTabBar::tab:selected { color: %1; border-bottom: 2px solid %1; }
    QGroupBox {
      background-color: %5; border: 1px solid %3; border-radius: 6px;
      margin-top: 14px; padding: 12px;
    }
    QGroupBox::title {
      subcontrol-origin: margin; left: 12px; padding: 0 4px; color: %4;
    }
    QPushButton {
      background-color: %5; border: 1px solid %3; border-radius: 4px;
      padding: 6px 14px;
    }
    QPushButton:hover { border-color: %4; }
    QPushButton:pressed { background-color: #eef0f2; }
    QTableWidget {
      background-color: %5; border: 1px solid %3; border-radius: 4px;
      gridline-color: #eef0f2;
    }
    QHeaderView::section {
      background-color: %5; border: none; border-bottom: 1px solid %3;
      padding: 6px; color: %4;
    }
    QLineEdit, QComboBox, QDateEdit, QSpinBox, QDoubleSpinBox {
      background-color: %5; border: 1px solid %3; border-radius: 4px;
      padding: 5px 8px;
    }
  )")
      .arg(QString::fromLatin1(kTinta), QString::fromLatin1(kFondo),
           QString::fromLatin1(kLinea), QString::fromLatin1(kTintaSuave),
           QString::fromLatin1(kPapel));
}

}  // namespace

int main(int argc, char** argv) {
  QApplication aplicacion(argc, argv);
  QApplication::setApplicationName(QStringLiteral("Sobres"));
  QApplication::setOrganizationName(QStringLiteral("Sobres"));
  QApplication::setApplicationDisplayName(QStringLiteral("Sobres"));
  // Los montos se formatean a mano en el núcleo, pero el calendario y los
  // campos numéricos de Qt siguen la configuración regional de México.
  QLocale::setDefault(QLocale(QLocale::Spanish, QLocale::Mexico));
  aplicacion.setStyleSheet(hojaDeEstiloGlobal());

  const QStringList argumentos = QApplication::arguments();
  const QString ruta = argumentos.size() > 1
                           ? argumentos.at(1)
                           : sobres::Repositorio::rutaPorOmision();

  sobres::Repositorio repositorio;
  QString error;
  if (!repositorio.abrir(ruta, &error)) {
    QMessageBox::critical(
        nullptr, QObject::tr("No se pudo abrir el archivo de datos"),
        QObject::tr("Ruta: %1\n\n%2").arg(ruta, error));
    return 1;
  }

  sobres::VentanaPrincipal ventana(&repositorio);
  ventana.show();
  return aplicacion.exec();
}
