#ifndef PARSER_H
#define PARSER_H

#include "path.hpp" // per Point, Path, Grid
#include <string>
#include <vector>
namespace pf {

struct id_to_symbol {
  int id;
  int symbol;
  // si potrebbe fare con una map (!!!)
  id_to_symbol() : id(0), symbol(0) {}

  id_to_symbol(int id_val, int symbol_val) : id(id_val), symbol(symbol_val) {}
};

struct oggetto_poligonale_vietato {
  int id;
  std::vector<Point> contorno;
  bool e_pieno;
};

// funzioni
size_t salta_virgolette_spazi(const std::string &linea, size_t inizio);
int trova_simbolo(const std::string &linea);
id_to_symbol ids_to_symbols(const std::string &linea);
std::vector<int>
trova_id_vietati_pieni(const std::vector<id_to_symbol> &ids_symbols,
                       const std::vector<int> &vietati_o_pieni);
double calcola_fattore_scala();
std::vector<Point> estrai_coordinate(const std::string &linea,
                                     double fattore_scala);
void calcola_dimensioni_max_min(
    const std::vector<oggetto_poligonale_vietato> &oggetti, double &min_x,
    double &max_x, double &min_y, double &max_y, const Point &arrivo,
    const Point &partenza);
void mondo_a_griglia(double x, double y, double min_x, double max_x,
                     double min_y, double max_y, int larghezza_griglia,
                     int altezza_griglia, int &gx, int &gy);
bool calcola_intersezione(int x1, int y1, int x2, int y2, int y, int &x);
void riempi_poligoni(std::vector<std::vector<bool>> &griglia,
                     const std::vector<std::pair<int, int>> &poligono,
                     int larghezza_griglia, int altezza_griglia);
void verde3_oltrepassabile(std::vector<int> &ids_vietati,
                           std::vector<int> &ids_pieni);
void oggetti_to_griglia(const oggetto_poligonale_vietato &oggetto,
                        std::vector<std::vector<bool>> &griglia, double min_x,
                        double max_x, double min_y, double max_y,
                        int larghezza_griglia, int altezza_griglia);
void dilata_ostacoli(std::vector<std::vector<bool>> &griglia);
double costo_path_geometrico_mondo(const std::vector<Point> &path, double min_x,
                                   double max_x, double min_y, double max_y,
                                   int larghezza_griglia, int altezza_griglia,
                                   double fattore_scala);

void esporta_percorso_omap(const std::string &filename_input,
                           const std::string &filename_output,
                           const std::vector<Path> &paths,
                           const std::vector<id_to_symbol> &ids_symbols,
                           double min_x, double max_x, double min_y,
                           double max_y, int larghezza_griglia,
                           int altezza_griglia, double fattore_scala);
std::vector<Path> helper_main(const std::string &nome_file);
} // namespace pf
#endif