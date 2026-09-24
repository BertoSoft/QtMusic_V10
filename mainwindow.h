#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qtmusic.h"

#include <QMainWindow>
#include <QListWidgetItem>
#include <QProgressBar>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;


private slots:
    // Slot que recibira el estado de QtMusic y llamara a dibujaUi()
    void dibujaUi(const QtMusic::EstadoUi &estado);

    void on_lstCanciones_itemClicked(QListWidgetItem *item);

    void on_btnPlay_clicked();

    void on_btnPause_clicked();

    void on_barraProgreso_sliderReleased();

    void on_barraProgreso_sliderPressed();

    void on_barraVolumen_valueChanged(int value);

    void on_btnProximaCancion_clicked();

    void on_btnAnterior_clicked();

    void on_btnSiguiente_clicked();

    void on_lstCanciones_itemDoubleClicked(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;

    // Funciones privadas
    void initUi();
    void initConnect();
    void initEqualizador();

    // Almacena dinámicamente los punteros a las 32 barras de la interfaz
    std::vector<QProgressBar*> m_barrasUi;


    // Variable que guarda el puntero a QtMusic
    QtMusic *m_qtMusic;
};
#endif // MAINWINDOW_H
