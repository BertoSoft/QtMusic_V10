#ifndef QTMUSIC_H
#define QTMUSIC_H

#include <QObject>
#include <QList>
#include <QString>
#include <QMediaPlayer>
#include <QAudioOutput>


class QtMusic:public QObject{

    Q_OBJECT

public:

    explicit QtMusic(QObject *parent = nullptr);

    // Definicion de estructuras y estados
    enum class EstadoPlayer{
        Stop,
        Play,
        Pause
    };

    struct Cancion{
        int         id;
        QString     nombre;
        QString     ruta;
    };

    struct EstadoUi{
        QList<Cancion>  listaCanciones;
        EstadoPlayer    estadoPlayer        = EstadoPlayer::Stop;
        QString         cancionActual       = "Ninguna canción seleccionada...";
        QString         proximaCancion      = "Ninguna canción seleccionada...";
        int             volumen             = 50;
        int             progreso            = 0;
        int             duracion            = 0;
    };

    // Funcion que usa Ui para recoger estado
    EstadoUi    getEstado() const;
    void        initQtMusic();
    void        setNuevaCancion(int id);
    void        setProximaCancio(int id);
    void        playClick();
    void        pauseClick();
    void        setPosicionBarraProgreso(int valor);
    void        setVolumen(int valor);

signals:

    // señal que se emite cada vez que cambia el estado
    void estadoActualizado(const QtMusic::EstadoUi &nuevoEstado);

private:

    // Variable par Player y SalidaSonido
    QMediaPlayer    *m_player       = nullptr;
    QAudioOutput    *m_audioOutput  = nullptr;

    // La variable real que mantien el estado en memoria
    EstadoUi m_estado;

    // Funciones Privadas
    QString nombreCancionToRuta(QString nombre);

};

#endif // QTMUSIC_H
