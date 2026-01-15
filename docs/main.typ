#import "@preview/diatypst:0.8.0": *
#let weak_mpi = csv("csv/weak_mpi.csv")
#let strong_mpi = csv("csv/strong_mpi.csv")
#let weak_omp = csv("csv/strong_openmp.csv")
#let strong_omp = csv("csv/weak_openmp.csv")
#let weak_multinode_mpi = csv("csv/weak_multinode_mpi.csv")
#let strong_multinode_mpi = csv("csv/strong_multinode_mpi.csv")

#let scaling_table(data) = {
  let fmt(val) = {
    if type(val) == str {
      let n = float(val)
      str(calc.round(n, digits: 4))
    } else {
      str(calc.round(val, digits: 4))
    }
  }

  align(center + horizon)[
    #block(width: 95%)[
      #set text(size: 1.1em)
      #table(
        columns: (1fr, 1fr, 1.5fr, 1.5fr, 1.2fr, 1.2fr),
        inset: (x: 12pt, y: 5pt), 
        align: center + horizon,
        stroke: 0.5pt + gray.lighten(40%),
        fill: (x, y) => 
          if y == 0 { blue.darken(70%) }
          else if calc.even(y) { gray.lighten(94%) } 
          else { white },

        [*#text(fill: white)[P]*], 
        [*#text(fill: white)[N]*], 
        [*#text(fill: white)[Tempo medio]*], 
        [*#text(fill: white)[Dev. standard]*], 
        [*#text(fill: white)[Speedup]*], 
        [*#text(fill: white)[Efficiency]*],
        
        ..data.slice(1).map(row => (
          row.at(0), 
          row.at(1), 
          fmt(row.at(2)), 
          fmt(row.at(3)), 
          fmt(row.at(4)), 
          fmt(row.at(5))
        )).flatten()
      )
    ]
  ]
}

#show: slides.with(
  title: "Binarizzazione di una matrice", 
  subtitle: "Progetto - Sistemi Concorrenti e Paralleli M",
  date: "A.A. 2025-2026",
  authors: ("Davide Chirichella - 0001222371"),
  ratio: 16/9,
  layout: "medium",
  title-color: blue.darken(70%),
  toc: true,
)

= Proposta di progetto

== Obiettivi
Lo scopo è quello di trasformare una matrice quadrata di numeri reali in una matrice binaria basata sulla media locale di ogni elemento.

- *Matrice di Input ($A$):* Quadrata di dimensione $N times N$, con $N >= 2000$.
- *Matrice di Output ($T$):* Quadrata di dimensione $N times N$. Contiene valori interi binari ${0, 1}$.

#v(1em)

// Griglia per affiancare matrice e spiegazione
#grid(
  columns: (2fr, 1.4fr), 
  gutter: 2em,
  
  align(center + horizon)[
  / Intorno ($I_(i j)$): Per ogni elemento $a_(i,j)$ della matrice $A$, si definisce un *intorno* come la sottomatrice $3 times 3$ centrata nell'elemento stesso:
    ],

    align(horizon)[
      #align(center)[
        #image("drawings/hpc-proj-intorno.drawio.png", width: 70%)
    ]
  ]
)

Il valore di ogni elemento $t_(i,j)$ della matrice risultante $T$ viene determinato seguendo questi passaggi:

1. *Media dell'intorno ($m_(i,j)$):* 
- Si calcola la media aritmetica di tutti i 9 elementi appartenenti all'intorno $I_(i j)$ con la formula: #align(center)[$ m_(i,j) = 1/9 sum_(x=i-1)^(i+1) sum_(y=j-1)^(j+1) a_(x,y) $] 

2. *Sogliatura binaria:*
   - Se $a_(i,j) > m_(i,j)$ allora $t_(i,j) = 1$
   - Se $a_(i,j) <= m_(i,j)$ allora $t_(i,j) = 0$

3. *Complessità:* 

  - Data la dimensione $N >= 2000$, l'algoritmo deve processare almeno $4 times 10^6$ elementi, rendendo l'ottimizzazione o la parallelizzazione rilevante.

= Implementazione Seriale 

== Allocazione delle matrici

Prima di affrontare il problema, è necessario comprendere il modo in cui la matrice viene linearizzata in memoria:

#v(1em)

#grid(
  columns: (1fr, 1.4fr), 
  gutter: 2em,
  
  align(center + horizon)[
    #set math.mat(column-gap: 0.8em)
    $ A = mat(
      a_(11), a_(12), dots, a_(1n);
      a_(21), a_(22), dots, a_(2n);
      dots.v, dots.v, dots.down, dots.v;
      a_(n 1), a_(n 2), dots, a_(n n);
    ) $
    #text(size: 0.8em, fill: gray)[Marice $n times n$]
  ],

  align(horizon)[
    *Memorizzazione Row-Major:* \
    La matrice viene "appiattita" riga dopo riga. Righe adiacenti logicamente sono rappresentate come contigue in memoria.

    *Rappresentazione in memoria:* \
    #block(
      fill: rgb("#f0f0f0"),
      inset: 5pt,
      radius: 4pt,
      stroke: gray.lighten(50%)
    )[
      $ [ [a_(11), dots, a_(1n)], [a_(21), dots, a_(2n)], dots, [a_(n 1), dots, a_(n n)] ] $
    ]
  ]
)

In una matrice $n times n$, l’elemento in posizione $(i, j)$ può essere acceduto linearmente come: `A[i * N + j]`, e viene allocata nel seguente modo:

```c
int *A = malloc(N * N * sizeof(int));
int *T = malloc(N * N * sizeof(int));
```

== Algoritmo
L'algoritmo seriale analizza ogni cella $A(i, j)$, calcola la media dei vicini (inclusa la cella stessa) e assegna un valore binario basato sul confronto tra il valore centrale e la media locale.


- Si cicla lungo le righe e le colonne, definendo la somma dei valori `sum` e il numero di elementi  `count`:
```c
for (i = 0; i < N; i++) {
  for (j = 0; j < N; j++) {
    sum = 0;
    count = 0;
```

E si definiscono i confini dell'intorno, stabilendo "fino a quanto" si può uscire dalla cella corrente:

```c
zmin = (i > 0) ? i - 1 : i;
zmax = (i < N - 1) ? i + 1 : i;
wmin = (j > 0) ? j - 1 : j;
wmax = (j < N - 1) ? j + 1 : j;
```

Si cicla successivamente tra i valori dell'intorno:
```c
for ( z = zmin; z <= zmax; z++) {
    for ( w = wmin; w <= wmax; w++) {
        sum += A[z * N + w];
        count++;

```
Infine, si calcola la soglia binaria: 

```c
T[i * N + j] = (A[i * N + j] * count > sum) ? 1 : 0
```

*Nota*: È presente una leggera *ottimizzazione*, ovvero l'uso della formula ($A[i][j] * "count" > "sum"$) al posto della divisione ($"sum""/""count"$).


= Implementazione Parallela: MPI

== Distribuzione del carico di lavoro

Poiché non è garantito che la dimensione della matrice $N$ sia un multiplo esatto del numero di processi $P$, in *MPI* occorre definire "artigianalmente" una strategia di partizionamento tra i processi.

- Il nodo *Master* adotta una logica basata sulla divisione intera e sul resto:

#grid(
  columns: (1fr, 1fr),
  gutter: 15pt,
  block(fill: rgb("#f0f0f0"), inset: 10pt, radius: 4pt, width: 100%)[
    *`base_rows`*: Ogni processo riceve almeno `N / num_proc` righe.
  ],
  block(fill: rgb("#f0f0f0"), inset: 10pt, radius: 4pt, width: 100%)[
    *`extra_rows`:* Le righe rimanenti (`N % num_proc`) vengono ridistribuite.
  ]
)

- Prima di inviare ai processi le righe di cui si occuperanno (mediante la primitiva `MPI_Scatterv`, poichè sono di dimensione variabile), il nodo *Master* popola due strutture dati che vengono mandate in Broadcast (`MPI_Bcast`) a tutti i processi allocati:

#grid(
  columns: (1fr, 1fr),
  gutter: 15pt,
  block(fill: rgb("#f0f0f0"), inset: 10pt, radius: 4pt, width: 100%)[
    *`sendcounts`*: Memorizza il numero totale di elementi assegnati a ogni processo.
  ],
  block(fill: rgb("#f0f0f0"), inset: 10pt, radius: 4pt, width: 100%)[
    *`senddispls`:* L’offset iniziale di ciascun processo, calcolato come somma del numero di elementi assegnati ai processi precedenti.
  ]
)

/ Esempio: Matrice: $5 times 5$, Numero di processi: $4$

#set text(size: 10pt)
#grid(
  columns: (1fr, 1fr),
  column-gutter: 20pt,
  [
    #align(left)[
      *1. MPI_Bcast* \
      Il master comunica a tutti quanto dovranno allocare (`sendcounts`, `senddispls`).
      #v(5pt)
      #image("drawings/hpc-proj-bcast.drawio.png", width: 67%)
    ]
  ],
  
  [
    #align(left)[
      *2. MPI_Scatterv* \
      Il master, dopo l'allocazione della matrice, invia i segmenti di dati reali alle memorie locali dei processi.
      #v(5pt)
      #image("drawings/hpc-proj-scatterv.drawio.png", width: 90%)
    ]
  ]
)

== Allocazione e scambio delle Ghost Rows

In un ambiente a #strong[memoria distribuita], ogni processo opera esclusivamente nel proprio spazio di indirizzamento isolato. 
Poiché non esiste memoria condivisa, il calcolo dei valori sui bordi richiede uno *scambio* esplicito di messaggi per ottenere i dati residenti sui nodi adiacenti.

Ogni processo alloca quindi localmente la propria porzione di righe (`my_rows = sendcounts[my_rank] / N`) estendendo la struttura dati con due righe supplementari:

#grid(
  columns: (1fr, 2fr),
  column-gutter: 2em,
  align: horizon,
  [
    - #strong[Upper Ghost Row]: Ospita l'ultima riga del processo precedente ($P_(k-1)$), se esiste.
    \
    - #strong[Lower Ghost Row]: Ospita la prima riga del processo successivo ($P_(k+1)$), se esiste.
  ],
  [
    #set align(center)
    #image("drawings/hpc-proj-matrix-scatter.drawio.png", width: 100%)
  ]
)

Successivamente, ogni processo:
1. Determina chi sono processi adiacenti:
```c
int up   = (my_rank > 0)            ? my_rank - 1 : MPI_PROC_NULL;
int down = (my_rank < num_proc - 1) ? my_rank + 1 : MPI_PROC_NULL;
```
2. Alloca la struttura dati per propria porzione di matrice con due righe extra:
```c
int total_rows = my_rows + 2;
int *my_A_plus_ghosts = calloc(total_rows * N, sizeof(int));
```

3. Definisce un puntatore per ogni sezione della memoria allocata:
```c
int *upper_ghost = my_A_plus_ghosts;                     // row -1
int *local_data  = my_A_plus_ghosts + N;                 // rows [0 ... my_rows-1]
int *lower_ghost = my_A_plus_ghosts + (my_rows + 1) * N; // row + my_rows
```

// Griglia per affiancare matrice e spiegazione
#grid(
  columns: (1fr, 2fr), 
  gutter: 1em,
  
  align(center + horizon)[
     #image("drawings/hpc-proj-A_plus_ghosts.drawio.png")
  ],
  
  align(horizon)[
    4. Il Master distribuisce le porzioni della matrice ai vari processi tramite una `MPI_Scatterv`. Ogni processo riceve `my_rows` righe (pari a `sendcounts[my_rank]` elementi), che vengono mappate direttamente nel buffer `local_data` del processo ricevente:
    ```c
    MPI_Scatterv(
        A, sendcounts, senddispls, MPI_INT,
        local_data, sendcounts[my_rank], MPI_INT,
        0, MPI_COMM_WORLD
    );
  ```
  ]
)


5. Per consentire l'invio e la ricezione delle Ghost Rows, si utilizzano le primitive di comunicazione punto-a-punto. Ci sono diversi approcci che possono essere implementati:

  - `MPI_Recv` (bloccante) + `MPI_Ssend`: implementano una comunicazione sincrona di tipo *rendez-vous*.
  - `MPI_Irecv` + `MPI_Isend`: permettono comunicazioni *asincrone*, richiedono `MPI_Wait`.
  - `MPI_Sendrecv`: esegue simultaneamente invio e ricezione, ottimizzando la sincronizzazione.


Di seguito è mostrata l'implementazione mediante `Ssend` e `Recv`:

#grid(
  columns: (2.2fr, .7fr),
  column-gutter: 10pt,
  align: horizon,
  [
1. Ogni processo (tranne il primo) invia la propria riga di bordo superiore al vicino `up`:
    ```c
if (my_rank > 0 && my_rows > 0) {
  MPI_Ssend(local_data, N, MPI_INT, up, 100, MPI_COMM_WORLD);
  MPI_Recv(upper_ghost, N, MPI_INT, up, 200, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
    ```
  ],
  [ #image("drawings/hpc-proj-ssend-recv-2.drawio.png", width: 100%)]
)

2. Successivamente, si calcola l'indice di memoria dove inizia l'ultima riga locale per l'invio al processo sottostante:

```c
int send_down_offset = (my_rows > 0 ? (my_rows - 1) * N : 0);
```
\
\
3. L'invio tramite `Ssend` sincronizza i processi: una volta completata la trasmissione della riga di confine, il processo viene "sbloccato" per la ricezione dal nodo superiore della Ghost Row necessaria:

#grid(
  columns: (2.7fr, 1.1fr),
  column-gutter: 10pt,
  align: horizon,
  [
    ```c
    if (my_rank < num_proc - 1 && my_rows > 0) {
        if (sendcounts[down] > 0) {
            MPI_Recv(lower_ghost, N, MPI_INT, down, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Ssend(local_data + send_down_offset, N, MPI_INT, down, 200, MPI_COMM_WORLD);
        }
    }
    ```
  ],
  [ #image("drawings/hpc-proj-ssend-recv-1.drawio.png", width: 70%)]
)

== Calcolo elementi dell'intorno e sogliatura

Dopo lo scambio dei dati, ogni processo itera sulla propria porzione di matrice. Si ricorda che:

#table(
  columns: (2.3fr, 2.3fr),
  inset: 3pt,
  stroke: gray.lighten(60%),

  [*Indice (riga logica)*], [*Contenuto*],
  [`riga 0`], [Ghost row superiore],
  [`righe 1 … my_rows`], [Righe di dati locali],
  [`riga my_rows + 1`], [Ghost row inferiore],
)

  Per compensare la presenza delle Ghost Rows, l'indice locale deve quindi essere traslato verticalmente:
```c
int zmin = (i == 0 && my_rank == 0) ? 0 : -1;                     // limite superiore
int zmax = (i == my_rows - 1 && my_rank == num_proc - 1) ? 0 : 1; // limite inferiore
```
  E orizzontalmente:
```c
int wmin = (j > 0) ? -1 : 0;    // limite destro
int wmax = (j < N - 1) ? 1 : 0; // limite sinistro
```


  #grid(
  columns: (2.2fr, 1fr), 
  gutter: 1em,
  
  align(left + horizon)[
    Nel calcolo della soglia locale, l'indice di riga viene calcolato come $i+1+"dz"$ per saltare correttamente la ghost row superiore.
      ```c
for (int dz = zmin; dz <= zmax; dz++) {
  int rowIndex = i + 1 + dz;
  
  for (int dw = wmin; dw <= wmax; dw++) {
    int colIndex = j + dw;
    
    sum += my_A_plus_ghosts[rowIndex * N +
    colIndex];
    
    count++;
  }
}

my_T[i * N + j] = (local_data[i * N + j] * count > sum)? 1 : 0;
```
  ],
  align(horizon)[
     #image("drawings/hpc-proj-matrix-for-cycle.drawio.png")
  ]
)

== Liberazione memoria allocata e raccolta risultati

Si libera la memoria allocata per la matrice locale con le Ghost Rows:

```c
free(my_A_plus_ghosts);
```
Il nodo Master invoca la primitiva `MPI_Gatherv` per collezionare i risultati parziali:
```c
MPI_Gatherv(
    my_T, (my_rows * N), MPI_INT,           
    T_raw, sendcounts, senddispls, MPI_INT,
    0, MPI_COMM_WORLD
);
```

Ed infine, tutti i nodi liberano la propria porzione di matrice e le strutture dati allocate in precedenza:
```c
if (my_rows > 0) 
  { free(my_T); }
free(sendcounts); free(senddispls);
```

= Implementazione Parallela: OpenMP

== Considerazioni generali

L'implementazione OpenMP è significativamente più semplice a causa della sua gestione dei dati:

- Tutti i thread vedono lo stesso spazio di indirizzamento (*Shared Memory*), ogni thread può leggere direttamente l'intorno di un punto a prescindere dalle righe che gli competono.

- Il compilatore si occupa di parallelizzare il codice mediante clausole singole `#pragma`

Poichè OpenMP segue il modello _Cobegin-Coend_, il Master thread dovrà creare dei worker thread al momento della direttiva `parallel`.

#align(center)[#image("drawings/hpc-proj-cobegin-coend.drawio.png", width: 70% )]

== Scelte implementative

Le scelte adottate per l'implementazione dell'algoritmo parallelo hanno riguardato: 

- *Visibilità delle Variabili*: 
  - Matrici $A$ e $T$: allocate fuori dal ciclo e dichiarate `shared` tra tutti i thread.
  - Variabili di supporto al calcolo locale ($i, j, "sum", "count", "etc..."$): dichiarate `private` (andranno successivamente inizializzate dai singoli thread).

  #align(center)[#image("drawings/hpc-proj-visibilita-variabili.drawio.png", width: 70% )]

- *Clausola `for`*: Permette di distribuire le iterazioni di un ciclo tra i thread, e si è scelto di farlo solo per il ciclo "esterno" (che itera sulle righe).

- *Scheduling dei thread*: è stato scelto lo `scheduling(static)` perché il carico computazionale di ogni iterazione è predeterminato e ogni thread lavora su righe contigue.

#grid(
  columns: (30%, 70%),
  gutter: 10pt,
  [
     #set align(horizon)
     - *Nota*: Se il numero di righe non è divisibile per i thread, OpenMP assegna le iterazioni rimanenti ai thread con rank *inferiore* per coprire l'intero dominio.
     
  / Esempio: Matrice: $4 times 4$\ Numero di thread: $3$
  ],
  [
    #set align(right)
    #image("drawings/hpc-proj-for.drawio.png", width: 100%)
  ]
)

== Algoritmo

Si definiscono le clausole OpenMP che permettono di stabilire:
- L'assegnazione statica delle righe nel ciclo *for*;
- Numero di *thread* per processo (come parametro del programma);
- Variabili *condivise*;
- Variabile *private* (non inizializzate).

```c
#pragma omp parallel for num_threads(num_threads) schedule(static) \
shared(A, T, N) private(i, j, sum, count, zmin, zmax, wmin, wmax, z, w)
```

L'implementazione dell'algoritmo è la medesima della versione sequenziale:

```c
for (i = 0; i < N; i++) {
  for (j = 0; j < N; j++) {
    sum = 0;
    count = 0;
    
    // ...
}
```

= Benchmark
== Setup sperimentale

L'obiettivo è quello di valutare la scalabilità dell'algoritmo al variare delle risorse di calcolo e della dimensione del problema. I tempi riportati rappresentano la *media* e la relativa *deviazione standard* di 3 misurazioni indipendenti per ogni configurazione.

Le due analisi effettuate riguardano:

1.  *Strong Scaling*: Dimensione del problema fissa ($N = 10.000$), unità di calcolo parallelo crescente ($P = 1 ... 48$).

2. *Weak Scaling*: Dimensione del problema ($N = 5000 sqrt(P)$) e unità di calcolo parallelo ($P = 1 ... 48$) crescenti.

Le metriche di valutazione utilizzate sono:

1. *Speedup ($S$)*: Rapporto tra il tempo in sequenziale e il tempo in parallelo.
  - Per lo *Strong Scaling*: $S(P) = T(1) / T(P)$ 
  - Per il *Weak Scaling* (Speedup Scalato): $S(P) = P dot (T(1) / T(P))$

2. *Efficiency ($E$)*: Rapporto tra speedup e unità di calcolo parallelo utilizzate: $E(P) = S(P) / P$

L’analisi sperimentale ha considerato tre configurazioni: *MPI intra-nodo* più processi MPI indipendenti su un singolo nodo), *MPI inter-nodo* (un processo MPI per nodo, con aumento progressivo del numero di nodi utilizzati) e *OpenMP* inter-nodo.

Le direttive *Slurm* utilizzate sulla macchina _Galileo-100_ del _Cineca_ sono le seguenti: 

#table(
  columns: (2fr, 0.8fr, 1fr, 1.6fr, 1.1fr),
  align: left,
  inset: (x: 10pt, y: 6pt),
  stroke: 0.5pt + gray.lighten(40%),
  fill: (x, y) =>
    if y == 0 { blue.darken(70%) }
    else if calc.even(y) { gray.lighten(94%) }
    else { white },

  [*#text(fill: white)[Configurazione]*],
  [*#text(fill: white)[Nodes]*],
  [*#text(fill: white)[Tasks]*],
  [*#text(fill: white)[CPU per Task]*],
  [*#text(fill: white)[N]*],

  [*MPI*, Strong scaling  intra-nodo],
  [1],
  [$1 ... 48$],
  [1],
  [$10.000$],

  [*MPI*, Weak scaling  intra-nodo],
  [1],
  [$1...48$ ],
  [1],
  [$5000 sqrt(P)$],

  [*MPI*, Strong scaling  inter-nodo],
  [$1...24$],
  [$1$ per nodo],
  [1],
  [$10.000$],


  [*MPI*, Weak scaling inter-nodo],
  [$1...24$],
  [$1$ per nodo],
  [1],
  [$5000 sqrt(P)$],

  [*OpenMP*, Strong scaling],
  [1],
  [1],
  [48 ($1...48$ thread)],
  [$10.000$],

  [*OpenMP*, Weak scaling],
  [1],
  [1 ],
  [48 ($1...48$ thread)],
  [$5000 sqrt(P)$],
)

*Nota*: Il numero di nodi indipendenti allocati sono limitati a 24 a causa di limiti del cluster.

== MPI
=== Strong Scaling intra-nodo

- Con $N$ fisso, all'aumentare di $P$, il numero di righe elaborate da ogni processo diminuisce, rendendo l'*overhead di comunicazione* più rilevante rispetto al tempo di calcolo puro.

#scaling_table(strong_mpi)

#figure(
  grid(
    columns: (1fr),
    gutter: 10pt,
    align(center)[#image("plot/mpi_strong.png", width: 80%)]
  ),
  caption: [Strong Scaling inter-nodo - MPI],
) <fig-omp-weak>

 

=== Weak Scaling intra-nodo

- Nel caso di *weak scaling* si osserva un comportamento analogo.

#scaling_table(weak_mpi)

#figure(
  grid(
    columns: (1fr),
    gutter: 10pt,
    align(center)[#image("plot/mpi_weak.png", width: 80%)]
  ),
  caption: [Weak Scaling inter-nodo - MPI],
) <fig-omp-weak>

=== Strong Scaling inter-nodo

- La similitudine tra il comportamento MPI *intra-nodo* e *inter-nodo* conferma il predominio dell’overhead di comunicazione rispetto al lavoro computazionale, che limita lo scaling in entrambe le configurazioni.


#scaling_table(strong_multinode_mpi)

#figure(
  grid(
    columns: (1fr),
    gutter: 10pt,
    align(center)[#image("plot/mpi_multinode_strong.png", width: 75%)]
  ),
  caption: [Strong Scaling intra-nodo - MPI],
) <fig-omp-weak>

=== Weak Scaling inter-nodo

- Si osserva anche qui un comportamento analogo al caso intra-nodo, con benefici di scaling limitati.

#scaling_table(weak_multinode_mpi)

#figure(
  grid(
    columns: (1fr),
    gutter: 10pt,
    align(center)[#image("plot/mpi_multinode_weak.png", width: 75%)]
  ),
  caption: [Weak Scaling intra-nodo - MPI],
) <fig-omp-weak>

== OpenMP

=== Strong Scaling

- OpenMP mantiene un'efficienza straordinaria, sempre superiore al *93%*. L'assenza di comunicazioni esplicite fa sì che i thread accedano *direttamente* alla memoria condivisa.

#scaling_table(weak_omp)

#figure(
  grid(
    columns: (1fr),
    gutter: 10pt,
    align(center)[#image("plot/omp_strong.png", width: 80%)]
  ),
  caption: [Strong Scaling - OpenMP],
) <fig-omp-weak>



=== Weak Scaling

- Il tempo di esecuzione rimane pressoché costante ($approx 0.77s$), poichè ogni core riceve la stessa *quantità* di lavoro computando in parallelo *senza conflitti*, e processando matrici più grandi nello stesso tempo di quelle piccole.

#scaling_table(strong_omp)

#figure(
  grid(
    columns: (1fr),
    gutter: 10pt,
    align(center)[#image("plot/omp_weak.png", width: 80%)]
  ),
  caption: [Weak Scaling - OpenMP],
) <fig-omp-weak>

#set align(center + horizon)

= Conclusioni

== Risultati ottenuti

#grid(
  columns: (1fr, 1fr),
  gutter: 20pt,
  align(left)[
    === OpenMP (Shared Memory)
    - *Speedup*: Quasi ideale (Efficienza sempre $> 95%$).
    
    - *Punti di forza*: Accesso diretto ai dati, zero overhead di comunicazione.
    
    - *Criticità*: Vincolato alle risorse di un singolo nodo.
  ],
  align(left)[
    === MPI (Distributed Memory)
    - *Speedup*: Limitato (Efficienza anche $< 20%$).
    
    - *Punti di forza*: Permette di eseguire su cluster multi-nodo per $N$ estremamente grandi.
    
    - *Criticità*: Performance limitate dall'overhead di comunicazione.
  ]
)

#v(2em)

#block(
  fill: luma(230),
  inset: 15pt,
  radius: 5pt,
  [
    In base ai calcoli effettuati, *OpenMP* risulta la soluzione più efficiente per la natura del problema. *MPI* è necessario solo quando la memoria del singolo nodo non è più sufficiente.
  ]
)

#pagebreak() // Forza una nuova pagina
#set page(header: none, footer: none) // Opzionale: rimuove eventuali numeri di pagina o header

#set align(center + horizon)
Questa slide è lasciata intenzionalmente vuota.

