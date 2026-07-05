// Per spiegare l'algoritmo viene prima data un'idea generale e poi qualche
// funzione viene trattata più nel dettaglio. Il pathfinding parte controllando
// se si può andare in linea retta verso l'arrivo e quindi controllando se sono
// liberi tutti i punti da partenza ad arrivo, utilizzando l'algoritmo di
// Bresenham. Se non sono liberi, l'algoritmo si ferma al primo ostacolo che
// incontra e trova il punto prima del primo ostacolo che chiameremo P_0. Si
// divide quindi nel percorso che gira in senso orario e in quello che gira in
// senso antiorario. Per ognuno dei due rami segue il bordo fino a che non vede
// spazio in direzione dell'arrivo, ovvero finchè il primo bool in direzione non
// è libero. Si salva quel punto che chiameremo punto_uscita_bordo. Segue il
// bordo all'indietro finchè non vede spazio in direzione della partenza e si
// salva quel punto che chiameremo punto_ingresso_bordo e tutti i punti del
// bordo tra punto_ingresso_bordo e punto_uscita_bordo. Crea il percorso formato
// da partenza, punto_ingresso_bordo , punto_uscita_bordo con tutti i punti
// della griglia della retta partenza-> punto_ingresso_bordo e tutti i punti del
// bordo.

// A questo punto viene riapplicata la funzione intera con, come partenza,
// punto_uscita_bordo e, come arrivo, sempre l'arrivo.

// Quando l'ultimo punto_uscita_bordo vedrà l'arrivo e non solo spazio in
// direzione si chiuderà il percorso che verrà pulito: verranno tolti tutti i
// nodi in eccesso e le deviazioni inutili. E se la retta punto_ingresso_bordo
// ->partenza o punto_ingresso_bordo -> punto_uscita_bordo_precedente non è
// completamente libera [perchè, ricordo, il mio algoritmo passa per P_0 (e non
// lo salva, il percorso dalla partenza va diretto a punto_ingresso_bordo )]
// viene riapplicato l'algoritmo con, come partenza, la partenza stessa e, come
// arrivo, il punto prima di quello che non vede punti precedenti, per essere
// certi di avere tutti i percorsi.

// Una volta che si hanno tutti i percorsi si ricalcola la lunghezza del
// percorso per sicurezza e si filtrano quelli lunghi più del 120\% del percorso
// più corto.

// Funzioni particolari
//  Per seguire il bordo c'è un movimento 8-direzionale con priorità. Quindi si
//  cerca da che parte è l'ostacolo e si prova prima ad andare in quella
//  direzione poi, se è in senso orario, per esempio, si girerà verso sinistra
//  fino a che non c'è una cella libera in quella direzione. Si controlla di
//  avere l'ostacolo vicino e, se si vede spazio in direzione dell'arrivo, si
//  prosegue.

// Per pulire i percorsi si parte dall'arrivo e si cerca il punto più lontano
// che vede, quindi partendo dalla partenza e poi avvicindandosi all'arrivo. Se
// se ne vede uno vengono tolti tutti i punti tra i due che si vedono. Se non se
// ne vede nessuno invece si applica nuovamente l'algoritmo come spiegato sopra.

// La funzione che pulisce viene applicata finchè non si ottiene due volte di
// fila lo stesso percorso. Questo serve per pulire anche la zona dell'unione
// tra i percorsi trovati apllicando l'algoritmo tra la partenza e quei punti
// che non vedono punti prima, e la fine del percorso. Infatti entrambi i tratti
// sono "puliti" ma può essere tolto qualcosa nell'unione.

#include "path.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <queue>
#include <vector>
namespace pf {
const int DX[4] = {0, 1, 0, -1};
const int DY[4] = {-1, 0, 1, 0};

// costruisce una griglia a partire da una matrice booleana (true = cella
// libera, false = ostacolo) inizializza matrice degli id degli ostacoli a -1
/*Griglia::Griglia(const std::vector<TipoCella> &g) : griglia(g) {
  assert(!g.empty());
  // for (std::size_t i = 1; i != altezza; i++) {
  // assert(g[i].size() == g[0].size());
  //}
  // assert(std::all_of(g.begin(), g.end(), [&](auto const& vec) {
  // return vec.size() == g[0].size();
  //}));
  altezza = static_cast<int>(g.size());
  larghezza = (altezza > 0) ? static_cast<int>(g[0].size()) : 0;
  id_ostacolo.resize(static_cast<size_t>(altezza),
                     std::vector<int>(static_cast<size_t>(larghezza), -1));
}*/

int Griglia::indice_monodimensionale(int x, int y) const{
  return y * larghezza + x;
}

bool Griglia::e_oltrepassabile(
    int x, int y) const { // ritorna valore cella, se nei margini
  if (x < 0 || x >= larghezza || y < 0 || y >= altezza) {
    return false;
  }
  return (griglia[indice_monodimensionale(x, y)] ==
          TipoCella::oltrepassabile);
}
bool Griglia::e_oltrepassabile(const Punto &p) const {
  return e_oltrepassabile(p.x, p.y);
}
int Griglia::ottieni_larghezza() const { return larghezza; }
int Griglia::ottieni_altezza() const { return altezza; }

void Griglia::rendi_non_oltrepassabile(int x, int y) {
  assert(x < larghezza && y < altezza && larghezza > 0 && altezza > 0);
  griglia[indice_monodimensionale(x, y)] = TipoCella::non_oltrepassabile;
}
// assegna un id univoco a ogni ostacolo, stesso ostacolo stesso id
void Griglia::controlla_id_ostacolo() {
  assert(larghezza > 0 && altezza > 0);
  int prossimo_id = 0;
  for (int y = 0; y != altezza; ++y) {
    for (int x = 0; x != larghezza; ++x) {
      if (!e_oltrepassabile(x, y) &&
          id_ostacolo[indice_monodimensionale(x, y)] == -1) {
        riempi(x, y, prossimo_id);
        ++prossimo_id;
      }
    }
  }
}

// restituisce id dell'ostacolo, -1 per il resto
int Griglia::ottieni_id_ostacolo(int x, int y) const {
  if (x < 0 || x >= larghezza || y < 0 || y >= altezza) {
    return -1;
  }
  return id_ostacolo[indice_monodimensionale(x, y)];
}
int Griglia::ottieni_id_ostacolo(const Punto &p) const {
  return ottieni_id_ostacolo(p.x, p.y);
}

Punto Griglia::ottieni_arrivo() const { return arrivo; }
Punto Griglia::ottieni_partenza() const { return partenza; }
double Griglia::ottieni_fattore_scala() const { return fattore_scala; }
void Griglia::inserisci_scala(double fattore_scala_calcolato) {
  fattore_scala = fattore_scala_calcolato;
}
int Griglia::ottieni_max_x() const { return max_x; }
int Griglia::ottieni_max_y() const { return max_y; }
int Griglia::ottieni_min_x() const { return min_x; }
int Griglia::ottieni_min_y() const { return min_y; }
void Griglia::inserisci_larghezza(int larghezza_calcolata) {
  larghezza = larghezza_calcolata;
}
void Griglia::inserisci_altezza(int altezza_calcolata) {
  altezza = altezza_calcolata;
}
void Griglia::inserisci_max_y(int max_y_calcolato) { max_y = max_y_calcolato; }
void Griglia::inserisci_min_y(int min_y_calcolato) { min_y = min_y_calcolato; }
void Griglia::inserisci_max_x(int max_x_calcolato) { max_x = max_x_calcolato; }
void Griglia::inserisci_min_x(int min_x_calcolato) { min_x = min_x_calcolato; }
void Griglia::inserisci_partenza(int x, int y) { inserisci_partenza({x, y}); }
void Griglia::inserisci_arrivo(int x, int y) { inserisci_arrivo({x, y}); }
void Griglia::inserisci_partenza(const Punto &p) { partenza = p; }
void Griglia::inserisci_arrivo(const Punto &p) { arrivo = p; }

void Griglia::forma_griglia_vettore(int altezza_calcolata,
                                    int larghezza_calcolata) {
  griglia.assign(
      static_cast<size_t>(altezza_calcolata*larghezza_calcolata),
                             TipoCella::oltrepassabile);
  id_ostacolo.assign(altezza_calcolata*larghezza_calcolata, -1);
}

// floodfill
void Griglia::riempi(int inizio_x, int inizio_y, int id) {
  assert(inizio_x >= 0 && inizio_x < larghezza);
  assert(inizio_y >= 0 && inizio_y < altezza);
  assert(id >= 0);
  std::queue<Punto> q;
  q.push(Punto(inizio_x, inizio_y));
  id_ostacolo[indice_monodimensionale(inizio_x, inizio_y)] =
      id;
  while (!q.empty()) {
    Punto p = q.front();
    q.pop();
    for (int d = 0; d != 4; ++d) {
      int nx = p.x + DX[d], ny = p.y + DY[d];
      if (nx >= 0 && nx < larghezza && ny >= 0 && ny < altezza) {
        if (!e_oltrepassabile(nx, ny) &&
            id_ostacolo[indice_monodimensionale(nx, ny)] ==
                -1) {
          id_ostacolo[indice_monodimensionale(nx, ny)] = id;
          q.push(Punto(nx, ny));
        }
      }
    }
  }
}

// aumenta di spessore di raggio 1 tutto per evitare gli attraversamenti in
// diagonale delle linee di spessore 1
void Griglia::dilata_ostacoli() {
  if (altezza < 1 || larghezza < 1) {
    return;
  }
auto nuova_griglia = griglia;
  for (int x = 0; x != larghezza; ++x) {
    for (int y = 0; y != altezza; ++y) {
      if (griglia[indice_monodimensionale(x, y)] == TipoCella::non_oltrepassabile) // cella vietata
      {
        for (int dy = -1; dy != 2; ++dy) {
          int ny = y + dy;
          for (int dx = -1; dx != 2; ++dx) {
            int nx = x + dx;

            if (nx >= 0 && nx < larghezza && ny >= 0 && ny < altezza) {
              nuova_griglia[indice_monodimensionale(nx, ny)] = TipoCella::non_oltrepassabile;
            }
          }
        }
      }
    }
  }
    griglia = std::move(nuova_griglia);
}

// linea dirtta da a a b con algoritmo bresenham
std::vector<Punto> linea_dritta_con_bresenham(const Punto &da, const Punto &a) {
  std::vector<Punto> punti;
  int x0 = da.x, y0 = da.y, x1 = a.x, y1 = a.y;
  int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
  int err = dx - dy;
  while (true) {
    punti.push_back(Punto(x0, y0));
    if (x0 == x1 && y0 == y1) {
      break;
    }
    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
  return punti;
}

// primo punto del primo ostacolo sotto la linea retta
Punto trova_primo_ostacolo_sulla_linea(const Punto &A, const Punto &B,
                                       const Griglia &griglia) {
  std::vector<Punto> linea = linea_dritta_con_bresenham(A, B);
  assert(!linea.empty());
  for (size_t i = 1; i != linea.size(); ++i) {
    Punto p = linea[i];
    if (p == B) {
      continue;
    }
    if (!griglia.e_oltrepassabile(p)) {
      return p;
    }
  }
  return Punto(-1, -1);
}

// ultimo punto libero prima dell'ostacolo
Punto trova_ultimo_punto_prima_del_primo_ostacolo(const Punto &A,
                                                  const Punto &B,
                                                  const Griglia &griglia) {
  std::vector<Punto> linea = linea_dritta_con_bresenham(A, B);
  assert(!linea.empty());
  Punto ultimo_libero = A;
  for (size_t i = 1; i != linea.size(); ++i) {
    if (!griglia.e_oltrepassabile(linea[i])) {
      return ultimo_libero;
    }
    ultimo_libero = linea[i];
  }
  return B;
}

// dal punto(P) c'è linea dritta libera verso arrivo(B)?
bool e_libero_fino_a_punto_di_arrivo(const Punto &P, const Punto &B,
                                     const Griglia &griglia) {
  std::vector<Punto> linea = linea_dritta_con_bresenham(P, B);
  assert(!linea.empty());
  for (size_t i = 1; i != linea.size(); ++i) {
    if (!griglia.e_oltrepassabile(linea[i])) {
      return false;
    }
  }
  return true;
}

// dal punto(P) c'è linea dritta libera per 1 bool in direzione
// dell'arrivo(B)(non entra in loop con concavi)
bool e_libero_spazio_in_direzione_arrivo(const Punto &P, const Punto &B,
                                         const Griglia &griglia,
                                         int id_ostacolo) {
  assert(id_ostacolo >= 0);
  std::vector<Punto> linea = linea_dritta_con_bresenham(P, B);
  assert(!linea.empty());
  for (size_t i = 1; i != linea.size(); ++i) {
    Punto p = linea[i];
    if (!griglia.e_oltrepassabile(p)) {
      int id_colpito = griglia.ottieni_id_ostacolo(p);
      if (id_colpito == id_ostacolo) {
        return false;
      } else {
        return true;
      }
    }
    if (p == B) {
      return true;
    }
  }
  return true;
}

//=============================================funzione segui il bordo
Bordo_e_uscita segui_il_bordo_in_un_verso_fino_a_uscita(
    const Punto &partenza, const Punto &destinazione, const Griglia &griglia,
    int id_ostacolo, senso_di_percorrenza direzione) {

  assert(id_ostacolo >= 0);
  assert(griglia.e_oltrepassabile(partenza));

  Bordo_e_uscita risultato{};

  const int MAX_PASSI = 4000; // limite di passi segui bordo

  // 8 direzioni
  static const int DX8[8] = {0, 1, 1, 1, 0, -1, -1, -1};
  static const int DY8[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

  auto diagonale_valida = [&](Punto da,
                              Punto a, // lambda per evitare tagli degli angoli
                              const Griglia &g) -> bool {
    int dx = a.x - da.x;
    int dy = a.y - da.y;

    // solo diagonali
    if (std::abs(dx) == 1 && std::abs(dy) == 1) {

      Punto p1(da.x + dx, da.y);
      Punto p2(da.x, da.y + dy);

      // taglio angoli
      if (!g.e_oltrepassabile(p1) && !g.e_oltrepassabile(p2)) {
        return false;
      }
    }

    return true;
  };

  Punto corrente = partenza;

  Percorso percorso;
  percorso.punti.push_back(corrente);

  int dir_precedente = -1;

  // trova primo orientamento rispetto all'ostacolo

  for (int d = 0; d != 8; ++d) {

    Punto vicino(partenza.x + DX8[d], partenza.y + DY8[d]);

    if (!griglia.e_oltrepassabile(vicino) &&
        griglia.ottieni_id_ostacolo(vicino) == id_ostacolo) {

      if (direzione == orario) {
        dir_precedente = (d + 6) % 8;
      } else {
        dir_precedente = (d + 2) % 8;
      }

      break;
    }
  }

  // non lo trova -> riprova a distanza 2

  if (dir_precedente == -1) {

    bool trovato = false;

    for (int d = 0; d != 8; ++d) {

      Punto vicino(partenza.x + DX8[d], partenza.y + DY8[d]);

      if (!griglia.e_oltrepassabile(vicino)) {
        continue;
      }

      if (!diagonale_valida(partenza, vicino, griglia)) {
        continue;
      }

      for (int k = 0; k != 8; ++k) {

        Punto adiacente(vicino.x + DX8[k], vicino.y + DY8[k]);

        if (!griglia.e_oltrepassabile(adiacente) &&
            griglia.ottieni_id_ostacolo(adiacente) == id_ostacolo) {

          corrente = vicino;
          percorso.punti.push_back(corrente);

          if (direzione == orario) {
            dir_precedente = (k + 6) % 8;
          } else {
            dir_precedente = (k + 2) % 8;
          }

          trovato = true;
          break;
        }
      }

      if (trovato) {
        break;
      }
    }

    if (!trovato) {
      return risultato;
    }
  }

  int passi = 0;

  // segui il bordo vero e proprio
  bool uscita_trovata = false;
  int passi_rimanenti = 0;
  while (passi < MAX_PASSI) {
    if (uscita_trovata) {

      --passi_rimanenti;

      if (passi_rimanenti == 0) {

        risultato.uscita = corrente;
        risultato.bordo = percorso;
        risultato.direzione = direzione;

        return risultato;
      }
    }
    // uscita valida
    if (!uscita_trovata && e_libero_spazio_in_direzione_arrivo(
                               corrente, destinazione, griglia, id_ostacolo)) {
      uscita_trovata = true;
      passi_rimanenti = 4;
    }

    int priorita[8];

    // priorità orario e antioriario, prima verso ostacolo, poi nella direzione
    // orario/antiorario

    if (direzione == orario) {

      priorita[0] = (dir_precedente + 2) % 8;
      priorita[1] = (dir_precedente + 1) % 8;
      priorita[2] = dir_precedente;
      priorita[3] = (dir_precedente + 7) % 8;
      priorita[4] = (dir_precedente + 6) % 8;
      priorita[5] = (dir_precedente + 5) % 8;
      priorita[6] = (dir_precedente + 4) % 8;
      priorita[7] = (dir_precedente + 3) % 8;
    }

    else {

      priorita[0] = (dir_precedente + 6) % 8;
      priorita[1] = (dir_precedente + 7) % 8;
      priorita[2] = dir_precedente;
      priorita[3] = (dir_precedente + 1) % 8;
      priorita[4] = (dir_precedente + 2) % 8;
      priorita[5] = (dir_precedente + 3) % 8;
      priorita[6] = (dir_precedente + 4) % 8;
      priorita[7] = (dir_precedente + 5) % 8;
    }

    bool spostato = false;

    for (int i = 0; i != 8; ++i) {

      int tdir = priorita[i];

      Punto prossimo(corrente.x + DX8[tdir], corrente.y + DY8[tdir]);

      // deve essere libero
      if (!griglia.e_oltrepassabile(prossimo)) {
        continue;
      }

      // evita attraversamento angoli
      if (!diagonale_valida(corrente, prossimo, griglia)) {
        continue;
      }

      // deve toccare l'ostacolo
      bool tocca_ostacolo = false;

      for (int k = 0; k != 8; ++k) {

        Punto adiacente(prossimo.x + DX8[k], prossimo.y + DY8[k]);

        if (!griglia.e_oltrepassabile(adiacente) &&
            griglia.ottieni_id_ostacolo(adiacente) == id_ostacolo) {

          tocca_ostacolo = true;
          break;
        }
      }

      if (!tocca_ostacolo) {
        continue;
      }

      // lo muove
      corrente = prossimo;

      percorso.punti.push_back(corrente);

      dir_precedente = tdir;

      spostato = true;

      break;
    }

    if (!spostato) {
      break;
    }

    ++passi;
  }

  return Bordo_e_uscita();
}

senso_di_percorrenza direzione_opposta(senso_di_percorrenza d) {
  return (d == orario) ? antiorario : orario;
}

double calcola_lunghezza_percorso(
    const std::vector<Punto> &percorso) { // per ogni punto fa pitagora tra
                                          // punto e quello prima, non è
  // costo effettivo ma mantiene proprzioni
  assert(percorso.size() >= 1);
  double costo = 0.0;
  for (size_t i = 1; i != percorso.size(); ++i) {
    costo += std::hypot(percorso[i].x - percorso[i - 1].x,
                        percorso[i].y - percorso[i - 1].y);
  }
  return costo;
}

// pulisce e genera nuovi percorsi se necessario
std::vector<std::vector<Punto>>
semplifica_percorso_all_indietro(const std::vector<Punto> &percorso,
                                 const Griglia &griglia) {

  assert(!percorso.empty());

  std::vector<Punto> corrente = percorso;

  if (corrente.size() <= 2) {
    return {corrente}; // se 2 punti non potrà mai tagliare
  }

  bool cambiato = true; // per vedere se continuare a pulire

  while (cambiato) {

    cambiato = false;

    int i = static_cast<int>(corrente.size()) - 1;

    while (i > 1) {

      int migliore = -1;

      // cerca il punto più vecchio visibile
      for (int j = 0; j != i; ++j) {
        if (e_libero_fino_a_punto_di_arrivo(corrente[static_cast<size_t>(i)],
                                            corrente[static_cast<size_t>(j)],
                                            griglia)) {
          migliore = j; // se lo trova esce e se lo salva
          break;
        }
      }

      if (migliore == i - 1) {
        --i;
        continue; // se arriva al punto prima e lo vede, tutto ok ma non taglia
                  // niente
      }

      if (migliore !=
          -1) { // se ne vede uno prima taglia, elimina tutti i punti in mezzo

        std::vector<Punto> nuovo;

        nuovo.insert(nuovo.end(), corrente.begin(),
                     corrente.begin() + migliore + 1);

        nuovo.push_back(corrente[static_cast<size_t>(i)]);

        if (!(i + 1 == static_cast<int>(corrente.size()))) {
          nuovo.insert(nuovo.end(), corrente.begin() + i + 1, corrente.end());
        }

        if (nuovo != corrente) {
          corrente = nuovo;
          cambiato = true;

          // riparti da capo sul nuovo percorso
          break;
        }

        i = migliore;
      } else { // se non vede nessun punto prima-> capita tra partenza/corrente
        // e punto_ingresso_bordo
        /*int best = 0;
        for (int k = i - 1; k != 0; --k) {
          if (griglia.e_oltrepassabile(corrente[k])) {
            best = k;
          }
        }*/
        std::cout << "BEST:" << griglia.e_oltrepassabile(corrente[/*best*/0]);
        std::cout << "BEST:" << griglia.e_oltrepassabile(corrente[i + 1]);
        std::vector<Percorso> percorsi_indietro = trova_percorsi(
            corrente[/*best*/0], corrente[static_cast<size_t>(i + 1)],
            griglia); // riapplica tutto l'algoritmo tra la partenza e il punto
                      // prima di quello che non vede altri punti dopo

        if (percorsi_indietro.empty()) {
          return {};
        }

        std::vector<std::vector<Punto>> risultati;

        for (const auto &pb : percorsi_indietro) {

          std::vector<Punto> finale = pb.punti;

          finale.insert(finale.end(), corrente.begin() + i + 1,
                        corrente.end()); // lo unisce ai punti che c'erano prima
          risultati.push_back(finale);
        }

        return risultati;
      }
    }
  }

  return {corrente};
}

std::vector<std::vector<Punto>> // funzione per pulire finchè un percorso cambia
semplifica_percorso_all_indietro_fino_a_stabilizzazione(
    const std::vector<Punto> &percorso, const Griglia &griglia) {
  assert(!percorso.empty());

  std::vector<std::vector<Punto>> correnti = {percorso};

  bool cambiato = true;

  while (cambiato) {
    cambiato = false;

    std::vector<std::vector<Punto>> nuovi;

    for (const auto &p : correnti) {
      auto puliti = semplifica_percorso_all_indietro(p, griglia);

      if (puliti.size() != 1 || puliti[0] != p) {
        cambiato = true;
      }

      nuovi.insert(nuovi.end(), puliti.begin(), puliti.end());
    }

    correnti.swap(nuovi); // per non copiare tutto
  }

  return correnti;
}

//=========================================funzione ricorsiva che crea i
// percorsi
void costruisci_percorsi_ricorsivo(
    const Punto &corrente, const Punto &destinazione, const Griglia &griglia,
    std::vector<Punto> &percorso_parziale, double costo_parziale,
    std::vector<Percorso> &percorsi_output, int profondita) {
  const int MAX_PROFONDITA = 8; // + di 8 volte rischia di esplodere
  assert(profondita >= 0);
  assert(corrente.e_valido() && destinazione.e_valido());

  if (profondita > MAX_PROFONDITA) {
    return;
  }
std::cout<<"oneeeeeeeeeeeeeeeeeeeeeeeeee\n";
  if (e_libero_fino_a_punto_di_arrivo(
          corrente, destinazione,
          griglia)) { // se vede l'arrivo pulisce e return
    std::vector<Punto> retta_finale{corrente, destinazione};
    Percorso p;
    p.punti = percorso_parziale;
    p.punti.insert(p.punti.end(), retta_finale.begin(), retta_finale.end());
    p.costo = costo_parziale + calcola_lunghezza_percorso(retta_finale);
    // percorsi_output.push_back(p);//da attivare se si toglie
    // semplifica_percorso_all_indietro_fino_a_stabilizzazione
    //   pulisci ritorna una o più varianti
    std::cout<<"dooooooooooooooooooooooooooooooos\n";
    auto varianti = semplifica_percorso_all_indietro_fino_a_stabilizzazione(
        p.punti, griglia);
    if (!varianti.empty()) {
      for (const auto &variante : varianti) {
        Percorso pv;
        pv.punti = variante;
        // pv.punti.insert(pv.punti.end(), retta_finale.begin(),
        //                  retta_finale.end());
        // pv.costo= costo_parziale + calcola_lunghezza_percorso(retta_finale);
        percorsi_output.push_back(pv);
      }
    }
    return;
  }

  Punto primo_colpito =
      trova_primo_ostacolo_sulla_linea(corrente, destinazione, griglia);
  if (!primo_colpito.e_valido()) {
    return;
  }
  assert(primo_colpito.e_valido());

  int id_ost = griglia.ottieni_id_ostacolo(primo_colpito);
  assert(id_ost >= 0);

  Punto ultimo_libero_prima_dell_ostacolo_in_direzione_arrivo =
      trova_ultimo_punto_prima_del_primo_ostacolo(corrente, destinazione,
                                                  griglia);
  bool adiacente = false;
  for (int d = 0; d != 8; ++d) { // controlla sa vicino a ostacolo
    static const int DX8[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const int DY8[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
    Punto nb(ultimo_libero_prima_dell_ostacolo_in_direzione_arrivo.x + DX8[d],
             ultimo_libero_prima_dell_ostacolo_in_direzione_arrivo.y + DY8[d]);
    if (!griglia.e_oltrepassabile(nb) &&
        griglia.ottieni_id_ostacolo(nb) == id_ost) {
      adiacente = true;
      break;
    }
  }
  if (!adiacente) { // altrimenti prova da primo_colpito le 4 direzioni e ne
                    // trova una dove è libero
    for (int d = 0; d != 4; ++d) {
      Punto nb(primo_colpito.x + DX[d], primo_colpito.y + DY[d]);
      if (griglia.e_oltrepassabile(nb)) {
        ultimo_libero_prima_dell_ostacolo_in_direzione_arrivo = nb;
        break;
      }
    }
  }
    std::cout<<"treeeeeeeeeeeeeeeeeeees\n";
  for (int d = 0; d != 2; ++d) { // per ogni senso di percorrenza
    senso_di_percorrenza dir = ((d == 0) ? orario : antiorario);
    senso_di_percorrenza dir_inversa = direzione_opposta(dir);
    Bordo_e_uscita uscita_dest = segui_il_bordo_in_un_verso_fino_a_uscita(
        ultimo_libero_prima_dell_ostacolo_in_direzione_arrivo, destinazione,
        griglia, id_ost,
        dir); // segui il bordo(fino a uscita(punto_uscita_bordo))
    if (!uscita_dest.uscita.e_valido()) {
      continue;
    }
    Punto punto_uscita_bordo = uscita_dest.uscita;

    Bordo_e_uscita uscita_corr = // torna indietro e trova punto_ingresso_bordo
        segui_il_bordo_in_un_verso_fino_a_uscita(punto_uscita_bordo, corrente,
                                                 griglia, id_ost, dir_inversa);
    if (!uscita_corr.uscita.e_valido()) {
      continue;
    }
    Punto punto_ingresso_bordo = uscita_corr.uscita;
    // punto_ingresso_bordo -> punto_uscita_bordo segue davvero il bordo
    std::cout<<"quartooooooooooooooooooooooooooooooooooooooos\n";
    std::vector<Punto> bordo_punto_ingresso_bordo_a_punto_uscita_bordo =
        uscita_corr.bordo.punti;
    std::reverse(bordo_punto_ingresso_bordo_a_punto_uscita_bordo.begin(),
                 bordo_punto_ingresso_bordo_a_punto_uscita_bordo.end());
    /*for (int i = 0; i !=
    static_cast<int>(bordo_punto_ingresso_bordo_a_punto_uscita_bordo.size());
    ++i) { std::cout << "(" <<
    bordo_punto_ingresso_bordo_a_punto_uscita_bordo[static_cast<size_t>(i)].x <<
    "; "
                <<
    bordo_punto_ingresso_bordo_a_punto_uscita_bordo[static_cast<size_t>(i)].y <<
    "), ";
    }*/

    // costruzione del percorso
    std::vector<Punto> nuovo_percorso =
        percorso_parziale; // è ricorsiva quindi parte già da un pezzo fatto

    // 1. corrente -> punto_ingresso_bordo (linea con tutti i punti della
    // griglia, non solo estremi)
    std::vector<Punto> segmento_corrente_punto_ingresso_bordo =
        linea_dritta_con_bresenham(corrente, punto_ingresso_bordo);
    if (!segmento_corrente_punto_ingresso_bordo.empty()) {
      if (!nuovo_percorso.empty() &&
          nuovo_percorso.back() ==
              segmento_corrente_punto_ingresso_bordo.front()) {
        segmento_corrente_punto_ingresso_bordo.erase(
            segmento_corrente_punto_ingresso_bordo.begin());
      }
      nuovo_percorso.insert(nuovo_percorso.end(),
                            segmento_corrente_punto_ingresso_bordo.begin(),
                            segmento_corrente_punto_ingresso_bordo.end());
    }

    // 2. bordo punto_ingresso_bordo -> punto_uscita_bordo (sempre tutti i
    // punti)
    if (!bordo_punto_ingresso_bordo_a_punto_uscita_bordo.empty()) {
      // evita duplicato tra fine segmento e inizio bordo
      if (!nuovo_percorso.empty() &&
          nuovo_percorso.back() ==
              bordo_punto_ingresso_bordo_a_punto_uscita_bordo.front()) {
        bordo_punto_ingresso_bordo_a_punto_uscita_bordo.erase(
            bordo_punto_ingresso_bordo_a_punto_uscita_bordo.begin());
      }

      nuovo_percorso.insert(
          nuovo_percorso.end(),
          bordo_punto_ingresso_bordo_a_punto_uscita_bordo.begin(),
          bordo_punto_ingresso_bordo_a_punto_uscita_bordo.end());
    }
        std::cout<<"cinqueeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\n";
    // calcola il costo, inutile tanto dopo verrà rifatto ma potrebbe diventarlo
    // se si filtra nel mentre
    double nuovo_costo = costo_parziale;

    // corrente -> punto_ingresso_bordo
    for (size_t i = 1; i != segmento_corrente_punto_ingresso_bordo.size(); ++i) {
          std::cout<<"vamosssssssssssssssssssssssss\n";
      nuovo_costo +=
          std::hypot(segmento_corrente_punto_ingresso_bordo[i].x -
                         segmento_corrente_punto_ingresso_bordo[i - 1].x,
                     segmento_corrente_punto_ingresso_bordo[i].y -
                         segmento_corrente_punto_ingresso_bordo[i - 1].y);
    }
    std::cout<<"seiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiis\n";
    // bordo punto_ingresso_bordo -> punto_uscita_bordo
    for (size_t i = 1;
         i != bordo_punto_ingresso_bordo_a_punto_uscita_bordo.size(); ++i) {
      nuovo_costo += std::hypot(
          bordo_punto_ingresso_bordo_a_punto_uscita_bordo[i].x -
              bordo_punto_ingresso_bordo_a_punto_uscita_bordo[i - 1].x,
          bordo_punto_ingresso_bordo_a_punto_uscita_bordo[i].y -
              bordo_punto_ingresso_bordo_a_punto_uscita_bordo[i - 1].y);
    }
std::cout<<"profondità: "<<profondita<<'\n';
    costruisci_percorsi_ricorsivo(punto_uscita_bordo, destinazione, griglia,
                                  nuovo_percorso, nuovo_costo, percorsi_output,
                                  profondita + 1);
  }
}

// se è dritto va, se no costruisci_percorsi_ricorsivo, ordina per costo da min
// a max, elimina quelli uguali
std::vector<Percorso> trova_percorsi(const Punto &A, const Punto &B,
                                     const Griglia &griglia) {

  assert(A.e_valido() && B.e_valido());
  assert(griglia.e_oltrepassabile(A) && griglia.e_oltrepassabile(B));
  std::vector<Percorso> risultati;
  if (e_libero_fino_a_punto_di_arrivo(A, B, griglia)) {
    Percorso diretto;
    diretto.punti = {A, B};
    diretto.costo =
        calcola_lunghezza_percorso(linea_dritta_con_bresenham(A, B));
    return {diretto};
  }
  std::vector<Punto> base{};
  costruisci_percorsi_ricorsivo(A, B, griglia, base, 0, risultati, 0);

  std::sort(
      risultati.begin(), risultati.end(),
      [](const Percorso &a, const Percorso &b) { return a.punti < b.punti; });
  risultati.erase(std::unique(risultati.begin(), risultati.end(),
                              [](const Percorso &a, const Percorso &b) {
                                return a.punti == b.punti;
                              }),
                  risultati.end());

  return risultati;
}

std::vector<Percorso> filtra_percorsi_per_lunghezza(
    std::vector<Percorso>
        &percorsi) { // ricalcola costi e trova min, poi filtra
  if (percorsi.empty()) {
    return {};
  }
  assert(!percorsi.empty());

  // ricalcola costi per sicurezza
  for (auto &p : percorsi) {
    p.costo = calcola_lunghezza_percorso(
        p.punti); // non è costo effettivo ma mantiene proprzioni quindi si può
                  // usare per filtrare
  }

  auto it = std::min_element(
      percorsi.begin(), percorsi.end(),
      [](const Percorso &a, const Percorso &b) { return a.costo < b.costo; });
  double costo_minimo = it->costo;
  double costo_massimo = costo_minimo * 1.2;

  std::vector<Percorso> filtrati;
  std::copy_if(percorsi.begin(), percorsi.end(), std::back_inserter(filtrati),
               [&](const Percorso &p) { return p.costo <= costo_massimo; });
  return filtrati;
}

std::vector<Percorso> // funzione wrapper
trova_percorsi_con_algoritmo_completo(const Punto &A, const Punto &B,
                                      Griglia &griglia) {
  std::cout << "innnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn";
  griglia.controlla_id_ostacolo();

  std::vector<Percorso> tutti_i_percorsi = trova_percorsi(A, B, griglia);
  std::vector<Percorso> percorsi =
      filtra_percorsi_per_lunghezza(tutti_i_percorsi);
  return percorsi;
}
} // namespace pf