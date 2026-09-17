#include "extracto.h"
#include "modelos.h"

namespace sobres{
    // Devuelve las líneas de un extracto del mes indicado:
//   - una línea por movimiento del mes, en orden cronológico
//   - cada línea lleva: fecha, tipo de movimiento, el sobre involucrado,
//     la categoría (si tiene), el monto formateado, y la nota
//   - al final, un pie con el total gastado, el total de entradas,
//     y una comparación contra el mes anterior
std::vector<std::string> extractoMensual(
    int anio, int mes,
    const std::vector<Sobre>& sobres,
    const std::vector<Categoria>& categorias,
    const std::vector<Movimiento>& movimientos){
        
    }
}