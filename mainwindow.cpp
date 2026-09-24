#include "mainwindow.h"
#include "ui_mainwindow.h"

#include"qtmusic.h"

#include <QTime>
#include <QListWidgetItem>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_qtMusic(new QtMusic(this))  // Creamos el objeto asignándole esta ventana como padre
{
    ui->setupUi(this);

    initUi();
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::initUi(){
    initEqualizador();
    initConnect();
    m_qtMusic->initQtMusic();
}

void MainWindow::initConnect(){

    connect(m_qtMusic, &QtMusic::estadoActualizado, this, &MainWindow::dibujaUi);

}

void MainWindow::initEqualizador(){
    const int numeroBarras = 32;
    m_barrasUi.reserve(numeroBarras);

    for (int i = 0; i < numeroBarras; ++i) {
        // 1. Instanciamos una nueva barra de progreso
        QProgressBar *bar = new QProgressBar(this);

        // 2. Replicamos las propiedades exactas que tenías en el XML
        bar->setOrientation(Qt::Vertical);
        bar->setTextVisible(false);
        bar->setValue(0);

        // Política de tamaño para que se autoajusten elásticamente al ancho de la pantalla
        bar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // 3. Lo añadimos al layout de la interfaz
        ui->horizontalLayout_3->addWidget(bar);

        // 4. Guardamos la referencia en nuestro vector privado
        m_barrasUi.push_back(bar);
    }
}

void MainWindow::dibujaUi(const QtMusic::EstadoUi &estado){

    // 1.- Lista de canciones
    if(ui->lstCanciones->count() != estado.listaCanciones.count()){
        ui->lstCanciones->clear();
        for(int i=0; i <estado.listaCanciones.count(); i++){
            QListWidgetItem *item = new QListWidgetItem(estado.listaCanciones[i].nombre);
            item->setData(Qt::UserRole, estado.listaCanciones[i].ruta);
            item->setData(Qt::UserRole + 1, estado.listaCanciones[i].id);
            ui->lstCanciones->addItem(item);
        }
    }

    // 2.- Proxima Cancion
    ui->lblProximaCancion->setText(estado.proximaCancion);

    // 3.- Cancion Actual
    ui->lblCancionActual->setText(estado.cancionActual);

    // 4.- Señalaizamos la cancionPlay selecionada
    int idPlay = m_qtMusic->getCancionFromNombre(estado.cancionActual).id;
    for(int i = 0; i<ui->lstCanciones->count(); i++){
        QListWidgetItem *item = ui->lstCanciones->item(i);
        int idLista = item->data(Qt::UserRole + 1).toInt();

        QFont font = item->font();

        if(idPlay == idLista){
            item->setForeground(QBrush(QColor("#6366F1"))); // Color destacado del tema
            font.setBold(true);
            item->setFont(font);
        }
        else{
            // Restaurar valores por defecto para el resto de canciones
            item->setForeground(QBrush(QColor("#E0E0E6")));
            font.setBold(false);
            item->setFont(font);
        }
    }

    // 5.- Barrra de progreso solo actualiza si no se pulsa
    ui->barraProgreso->setMaximum(estado.duracion);
    ui->barraProgreso->setValue(estado.progreso);

    // 6.- Tiempos
    QString tiempoActual    = QTime(0,0,0).addSecs(estado.progreso).toString("mm:ss");
    QString tiempoTotal     = QTime(0,0,0).addSecs(estado.duracion).toString("mm:ss");

    ui->lblTiempo->setText(QString("%1 / %2").arg(tiempoActual).arg(tiempoTotal));

    // 7.- barra de volumen
    ui->barraVolumen->setValue(estado.volumen);

    // 8.- Barras equalizador (Simplificado con bucle dinámico)
    // Actualización dinámica de las 32 barras
    size_t limite = std::min(m_barrasUi.size(), static_cast<size_t>(estado.barrasEqualizador.size()));
    for (size_t i = 0; i < limite; ++i) {
        m_barrasUi[i]->setValue(estado.barrasEqualizador[i]);
    }
}

void MainWindow::on_lstCanciones_itemClicked(QListWidgetItem *item){
    int id = item->data(Qt::UserRole + 1).toInt();
    m_qtMusic->setProximaCancio(id);
}

void MainWindow::on_lstCanciones_itemDoubleClicked(QListWidgetItem *item){
    int id = item->data(Qt::UserRole + 1).toInt();
    m_qtMusic->setNuevaCancion(id);
}

void MainWindow::on_btnPlay_clicked(){
    m_qtMusic->playClick();
}

void MainWindow::on_btnPause_clicked(){
    m_qtMusic->pauseClick();
}

void MainWindow::on_barraProgreso_sliderReleased(){
    int newPosicion = ui->barraProgreso->value();
    m_qtMusic->setPosicionBarraProgreso(newPosicion);
}

void MainWindow::on_barraProgreso_sliderPressed(){
    m_qtMusic->pauseClick();
}

void MainWindow::on_barraVolumen_valueChanged(int value){
    m_qtMusic->setVolumen(value);
}

void MainWindow::on_btnProximaCancion_clicked(){
    QList<QListWidgetItem*> listaItem = ui->lstCanciones->selectedItems();

    if(listaItem.count()>0){
        int id =listaItem[0]->data(Qt::UserRole + 1).toInt();
        m_qtMusic->setNuevaCancion(id);
    }
}


void MainWindow::on_btnAnterior_clicked(){
    m_qtMusic->atrasClick();
}

void MainWindow::on_btnSiguiente_clicked(){
    m_qtMusic->adelanteClick();
}





