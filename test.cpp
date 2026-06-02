#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.hpp"
#include "path.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

using namespace pf;
// costruisce griglia di larghezza W e altezza H tutta libera
static std::vector<std::vector<bool>> griglia_libera(int W, int H) {
  return std::vector<std::vector<bool>>(
      static_cast<size_t>(H), std::vector<bool>(static_cast<size_t>(W), true));
}

static std::vector<Path> esegui_test_omap(const std::string &nome_file,
                                          double scala = 4000.0,
                                          char verde3 = 'n') {

  std::string simulated_input =
      std::to_string(scala) + "\n" + std::string(1, verde3) + "\n";

  std::istringstream input_stream(simulated_input);

  auto old_cin_buf = std::cin.rdbuf();
  std::cin.rdbuf(input_stream.rdbuf());

  std::vector<Path> paths = helper_main(nome_file);

  std::cin.rdbuf(old_cin_buf);

  return paths;
}

TEST_CASE("filtra_paths: tutti i percorsi entro 1.2x il costo minimo") {
  int W = 15, H = 15;
  auto griglia = griglia_libera(W, H);

  // Ostacolo orizzontale con due varchi
  for (int x = 2; x <= 12; ++x) {
    if (x != 5 && x != 10) // varchi a x=5 e x=10
    {
      griglia[7][static_cast<size_t>(x)] = false;
    }
  }

  Point A(7, 0), B(7, 14);
  auto paths = final_aggiratore(A, B, griglia);

  if (!paths.empty()) {
    double costo_min = paths[0].cost;
    for (const auto &p : paths) {
      costo_min = std::min(costo_min, p.cost);
    }

    for (const auto &p : paths) {
      CHECK(p.cost <= doctest::Approx(costo_min * 1.2).epsilon(0.01));
    }
  }
}

TEST_CASE("Grid::is_free: celle libere e occupate") {
  int W = 5, H = 5;
  auto griglia = griglia_libera(W, H);
  griglia[2][3] = false; // occupa (x=3, y=2)

  Grid grid(griglia);

  CHECK(grid.is_free(0, 0) == true);
  CHECK(grid.is_free(3, 2) == false);  // cella occupata
  CHECK(grid.is_free(-1, 0) == false); // fuori bounds
  CHECK(grid.is_free(0, 99) == false); // fuori bounds
}

TEST_CASE(
    "Grid::controlla_id_ostacolo assegna id diversi a ostacoli separati") {
  int W = 10, H = 5;
  auto griglia = griglia_libera(W, H);

  griglia[1][1] = false;

  griglia[3][8] = false;

  Grid grid(griglia);
  grid.controlla_id_ostacolo();

  int id1 = grid.get_id_ostacolo(1, 1);
  int id2 = grid.get_id_ostacolo(8, 3);

  CHECK(id1 >= 0);
  CHECK(id2 >= 0);
  CHECK(id1 != id2); // ostacoli distinti -> id diversi
}

TEST_CASE("Grid::controlla_id_ostacolo: blocco connesso ha lo stesso id") {
  int W = 10, H = 5;
  auto griglia = griglia_libera(W, H);

  for (int y = 1; y <= 3; ++y) {
    for (int x = 2; x <= 4; ++x) {
      griglia[static_cast<size_t>(y)][static_cast<size_t>(x)] = false;
    }
  }

  Grid grid(griglia);
  grid.controlla_id_ostacolo();

  int id_ref = grid.get_id_ostacolo(2, 1);
  CHECK(id_ref >= 0);
  CHECK(grid.get_id_ostacolo(3, 2) == id_ref);
  CHECK(grid.get_id_ostacolo(4, 3) == id_ref);
}

TEST_CASE("Costo percorso orizzontale 4 passi") {
  auto griglia = griglia_libera(10, 10);
  Point A(0, 5), B(4, 5);

  auto paths = final_aggiratore(A, B, griglia);
  REQUIRE(!paths.empty());

  // Deve trovare almeno un percorso con costo ~ 4
  bool trovato = false;
  for (const auto &p : paths) {
    if (p.cost == doctest::Approx(4.0).epsilon(0.1)) {
      trovato = true;
    }
  }

  CHECK(trovato);
}

TEST_CASE("Nessun percorso se arrivo è completamente circondato") {
  int W = 7, H = 7;
  auto griglia = griglia_libera(W, H);

  // Circonda B=(3,3) con un anello di ostacoli
  for (int y = 2; y <= 4; ++y) {
    for (int x = 2; x <= 4; ++x) {
      if (!(x == 3 && y == 3)) {
        griglia[static_cast<size_t>(y)][static_cast<size_t>(x)] = false;
      }
    }
  }

  Point A(0, 0), B(3, 3);
  auto paths = final_aggiratore(A, B, griglia);

  CHECK(paths.empty());
}

TEST_CASE("linea_dritta produce punti collineari e corretta lunghezza") {
  Point da(0, 0), a(4, 0);
  auto linea = linea_dritta(da, a);

  // Deve contenere esattamente 5 punti (0,1,2,3,4)
  CHECK(linea.size() == 5);
  CHECK(linea.front() == da);
  CHECK(linea.back() == a);

  // Tutti sulla stessa y
  for (const auto &p : linea) {
    CHECK(p.y == 0);
  }
}

TEST_CASE("Griglia rettangolare") {
  auto griglia = griglia_libera(20, 10); // 20 larghezza, 10 altezza
  Point A(0, 5), B(19, 5);
  auto paths = final_aggiratore(A, B, griglia);
  CHECK(!paths.empty());
  // verifica che i punti non escano dai bounds
}

TEST_CASE("Ostacolo a U, uscita verso l'alto") {
  int W = 15, H = 15;
  auto griglia = griglia_libera(W, H);
  // pareti sinistra, destra e fondo della U
  for (size_t y = 5; y <= 12; ++y) {
    griglia[y][5] = false; // parete sinistra
    griglia[y][9] = false; // parete destra
  }
  for (size_t x = 5; x <= 9; ++x) {
    griglia[12][x] = false;
  } // fondo
  // partenza dentro la U (6,7), arrivo fuori in alto (6,2)
  Point A(6, 7), B(6, 2);
  auto paths = final_aggiratore(A, B, griglia);
  CHECK(!paths.empty());
}

TEST_CASE("Griglia 1x1") {
  auto griglia = griglia_libera(1, 1);
  Point A(0, 0), B(0, 0);
  auto paths = final_aggiratore(A, B, griglia);
  CHECK(paths.size() == 1);
  CHECK(paths[0].cost == 0.0);
}

TEST_CASE("Filtra_paths esclude percorsi troppo lunghi") {
  // vettore di Path con costi noti
  std::vector<Path> percorsi;

  Path p1, p2, p3;
  p1.cost = 100.0;
  p2.cost = 119.0;
  p3.cost = 121.0; // fuori soglia

  // punti fittizi per sicurezza
  p1.points = {Point(0, 0), Point(10, 0)};
  p2.points = {Point(0, 0), Point(12, 0)};
  p3.points = {Point(0, 0), Point(14, 0)};

  percorsi.push_back(p1);
  percorsi.push_back(p2);
  percorsi.push_back(p3);

  auto filtrati = filtra_paths(percorsi);

  CHECK(filtrati.size() == 2);
  // Ordine non specificato, ma i costi devono essere <= 120
  for (const auto &p : filtrati) {
    CHECK(p.cost <= 120.0 + 1e-9);
  }
  // Nessuno dei filtrati deve avere costo 121
  for (const auto &p : filtrati) {
    CHECK(p.cost != doctest::Approx(121.0));
  }
}

TEST_CASE(
    "pulisci_all_indietro raddrizza un percorso zigzag su griglia libera") {
  int W = 20, H = 20;
  auto griglia = griglia_libera(W, H);
  Grid grid(griglia);

  // percorso a zigzag: (0,0) -> (5,5) -> (10,0) -> (15,5) -> (20,0)
  // griglia libera, quindi soluzione è linea retta.
  std::vector<Point> zigzag = {Point(0, 0), Point(5, 5), Point(10, 0),
                               Point(15, 5), Point(20, 0)};

  auto puliti = pulisci_all_indietro(zigzag, grid);

  // 1 sola versione
  CHECK(puliti.size() == 1);
  const auto &percorso_pulito = puliti[0];

  // primo e ultimo non camabiati
  CHECK(percorso_pulito.front() == zigzag.front());
  CHECK(percorso_pulito.back() == zigzag.back());

  // non deve contenere punti dello zigzag
  bool ha_punti_inutili = false;
  for (const auto &pt : percorso_pulito) {
    if (pt.x == 5 || pt.x == 10 || pt.x == 15) {
      if (pt.y != 0) {
        ha_punti_inutili = true;
      }
    }
  }
  CHECK(!ha_punti_inutili);
}

TEST_CASE("pulisci_fino_a_stabile converge a percorso ottimo") {
  int W = 20, H = 20;
  auto griglia = griglia_libera(W, H);
  Grid grid(griglia);

  // Stesso zigzag
  std::vector<Point> zigzag = {Point(0, 0), Point(5, 5), Point(10, 0),
                               Point(15, 5), Point(20, 0)};

  auto stabili = pulisci_fino_a_stabile(zigzag, grid);
  CHECK(stabili.size() == 1);
  const auto &finale = stabili[0];

  CHECK(finale.size() <= 3);
  for (const auto &pt : finale) {
    CHECK(pt.y == 0);
  }
}

TEST_CASE("vede_punto_di_arrivo: linea libera ritorna true") {
  int W = 10, H = 10;
  auto griglia = griglia_libera(W, H);
  Grid grid(griglia);

  Point P(0, 0), B(9, 9);
  CHECK(vede_punto_di_arrivo(P, B, grid) == true);
}

TEST_CASE("vede_punto_di_arrivo: ostacolo sulla linea ritorna false") {
  int W = 10, H = 10;
  auto griglia = griglia_libera(W, H);

  griglia[5][5] = false;
  Grid grid(griglia);

  Point P(0, 0), B(9, 9);
  CHECK(vede_punto_di_arrivo(P, B, grid) == false);
}

TEST_CASE("vede_punto_di_arrivo: ostacolo leggermente spostato non blocca") {
  int W = 10, H = 10;
  auto griglia = griglia_libera(W, H);

  griglia[6][5] = false;
  Grid grid(griglia);

  Point P(0, 0), B(9, 9);
  // bresenham da (0,0) a (9,9) passa per (5,5), non per (5,6)
  // quindi deve essere ancora libera
  CHECK(vede_punto_di_arrivo(P, B, grid) == true);
}

TEST_CASE("Mappa con tante ricorsioni anche mentre controlla all'indetro") {
  auto paths = esegui_test_omap("test_su_omap1.omap");
  CHECK(!paths.empty());
  CHECK(paths.size() == 10);
  auto it = std::min_element(
      paths.begin(), paths.end(),
      [](const Path &a, const Path &b) { return a.cost < b.cost; });
  double costo_minimo = it->cost;
  CHECK(costo_minimo == doctest::Approx(889).epsilon(1));
}

TEST_CASE("Mappa con 1 ostacolo in mezzo") {
  auto paths = esegui_test_omap("test_su_omap2.omap");
  CHECK(!paths.empty());
  CHECK(paths.size() == 2);
  auto it = std::min_element(
      paths.begin(), paths.end(),
      [](const Path &a, const Path &b) { return a.cost < b.cost; });
  double costo_minimo = it->cost;
  CHECK(costo_minimo == doctest::Approx(832).epsilon(1));
}

TEST_CASE("Mappa con linee") {
  auto paths = esegui_test_omap("test_su_omap3.omap");
  CHECK(!paths.empty());
  CHECK(paths.size() == 3);
  auto it = std::min_element(
      paths.begin(), paths.end(),
      [](const Path &a, const Path &b) { return a.cost < b.cost; });
  double costo_minimo = it->cost;
  CHECK(costo_minimo == doctest::Approx(1019).epsilon(1));
}

TEST_CASE("Poligono concavo") {
  auto paths = esegui_test_omap("test_su_omap4.omap");
  CHECK(!paths.empty());
  CHECK(paths.size() == 2);
  auto it = std::min_element(
      paths.begin(), paths.end(),
      [](const Path &a, const Path &b) { return a.cost < b.cost; });
  double costo_minimo = it->cost;
  CHECK(costo_minimo == doctest::Approx(889).epsilon(1));
}

TEST_CASE(
    "Tanti ostacoli da aggirare in avanti con un percorso poco sopra il 20%") {
  auto paths = esegui_test_omap("test_su_omap5.omap");
  CHECK(!paths.empty());
  CHECK(paths.size() == 7);
  auto it = std::min_element(
      paths.begin(), paths.end(),
      [](const Path &a, const Path &b) { return a.cost < b.cost; });
  double costo_minimo = it->cost;
  CHECK(costo_minimo == doctest::Approx(925).epsilon(1));
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TESTO D'ESEMPIO PER PROVARE NON CON MAPPPER

/*
<symbol type="16" id="136" code="521" name="Building"><description>A building is
a relatively permanent construction having a roof. Buildings within symbol Area
that shall not be entered (520) may just be represented in a simplified manner.
Areas totally contained within a building shall be mapped as being a part of the
building. A building shall not be entered. &lt;b&gt;The minimum gap between
buildings and other impassable features shall be 0.40 mm. Boundaries in touching
buildings shall not be represented.&lt;/b&gt; <symbol type="1" id="160"
code="701" name="Start"><description>The start or map issue point (if not at the
start) is shown by an equilateral triangle which points in the direction of the
first control. <symbol type="1" id="166" code="706"
name="Finish"><description>The finish is shown by two concentric
circles.</description><point_symbol inner_radius="2500" inner_color="-1"
outer_width="350" outer_color="5" elements="1"><element><symbol type="1"
code=""><point_symbol inner_radius="3500" inner_color="-1" outer_width="350"
outer_color="5" elements="0"/></symbol><object type="0"><coords count="1">0
0;</coords></object></element></point_symbol></symbol> <symbol type="2" id="167"
code="707" name="Marked route"><description>A marked route is shown on the map
with a dashed line.</description><line_symbol color="0" line_width="350"
minimum_length="0" join_style="1" cap_style="0" start_offset="0" end_offset="0"
dashed="true" segment_length="4000" end_length="0"
show_at_least_one_symbol="true" minimum_mid_symbol_count="0"
minimum_mid_symbol_count_when_closed="0" dash_length="2000" break_length="750"
dashes_in_group="1" in_group_break_length="500" mid_symbols_per_spot="1"
mid_symbol_distance="0"/></symbol> <symbol type="2" id="168" code="708"
name="Out-of-bounds boundary"><description>An out-of-bounds boundary shall not
be crossed. It shall be used for temporary uncrossable boundaries used for the
course setting.</description><line_symbol color="5" line_width="1000"
minimum_length="0" join_style="1" cap_style="0" start_offset="0" end_offset="0"
segment_length="4000" end_length="0" show_at_least_one_symbol="true"
minimum_mid_symbol_count="0" minimum_mid_symbol_count_when_closed="0"
dash_length="4000" break_length="1000" dashes_in_group="1"
in_group_break_length="500" mid_symbols_per_spot="1"
mid_symbol_distance="0"/></symbol>
</symbols>
<parts count="1" current="0">
<part name="Parte di default"><objects count="4">

Modificare gli oggetti qui sotto

<object type="1" symbol="136"><coords count="5">-1000 2000;0 2000;1000 0;-1000
-1000;-1000 2000 18;</coords><pattern rotation="0"><coord x="0"
y="0"/></pattern></object> <object type="1" symbol="136"><coords count="5">-4000
3000;-1000 3000;-3000 1000;-5000 1000;-4000 3000 18;</coords><pattern
rotation="0"><coord x="0" y="0"/></pattern></object> <object type="0"
symbol="160"><coords count="1">8000 -8000;</coords></object> <object type="0"
symbol="166"><coords count="1">-7000 5000;</coords></object>

Modificare/rimuovere fino a qui

"</objects>
</parts>
<templates count="0" first_front_template="0">
<defaults use_meters_per_pixel="true" meters_per_pixel="0" dpi="0" scale="0"/>
</templates>
</undo>
<redo>
</redo>
</barrier>
</map>
*/
