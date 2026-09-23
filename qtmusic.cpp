
#include "qtmusic.h"

#include <QObject>
#include <QStandardPaths>
#include <QDir>

QtMusic::QtMusic(QObject *parent): QObject(parent){

    // Iniciamos m_player
    m_audioOutput   = new QAudioOutput(this);
    m_player        = new QMediaPlayer(this);

    m_player->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(m_estado.volumen / 100.0);

    // iniciaamos los connect
    // la duracion de la cancion
    connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 duracionMs){
        m_estado.duracion = static_cast<int>(duracionMs / 1000);
        emit estadoActualizado(m_estado);
    });

    // el progreso de la cancion
    connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 progresoMs){
        m_estado.progreso = static_cast<int>(progresoMs / 1000);

        // si llegamos al final pasamos a la siguiente cancion, si la hay
        if(m_estado.progreso == m_estado.duracion){
            adelanteClick();
        }

        // Avisamos de cambio de estado
        emit estadoActualizado(m_estado);
    });


}

QtMusic::EstadoUi QtMusic::getEstado() const{
    return m_estado;
}

void QtMusic::initQtMusic(){

    // Obtenemos la lista de canciones
    QStringList     lstDirectorios;
    QStringList     lstFiltros;
    int             id;

    lstDirectorios << QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    lstDirectorios << QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);

    lstFiltros << "*.mp3" << "*.wav" << "*.ogg" << "*.flac";

    id = 0;
    // Recorremos los distintos directorios
    const QStringList &lstDirsCte = lstDirectorios;
    for(const QString &ruta: lstDirsCte){
        if(ruta.isEmpty()) continue;

        QDir        directorio      = QDir(ruta);
        QStringList lstArchivos     = directorio.entryList(lstFiltros, QDir::Files);

        // Recorremos los distintos archivos
        const QStringList lstArchivosCte = lstArchivos;
        for(const QString &nombre: lstArchivosCte   ){
            Cancion     nuevaCancion;
            bool        existe = false;

            nuevaCancion.nombre = nombre;
            nuevaCancion.ruta   = directorio.absoluteFilePath(nombre);

            // Recorremos los distintas canciones existentes
            const QList<Cancion> &lstCancionesCte = m_estado.listaCanciones;
            for(const Cancion &cancion: lstCancionesCte){
                if(cancion.nombre == nombre){
                    existe = true;
                    break;
                }
            }

            if(!existe){
                nuevaCancion.id = id;
                m_estado.listaCanciones.append(nuevaCancion);
                id++;
            }
        }
    }

    // Si la lista existe colocamos la primera
    if(m_estado.listaCanciones.count()>0){
        m_estado.cancionActual  = m_estado.listaCanciones[0].nombre;
        m_estado.proximaCancion = m_estado.listaCanciones[0].nombre;
    }

    // Emitimos la señal
    emit estadoActualizado(m_estado);

}

void QtMusic::setNuevaCancion(int id){
    Cancion cancion = getCancionFromId(id);

    if(!cancion.nombre.isEmpty()){

        m_player->setSource(QUrl::fromLocalFile(cancion.ruta));
        m_player->play();

        m_estado.cancionActual = cancion.nombre;
        m_estado.estadoPlayer = EstadoPlayer::Play;

        emit estadoActualizado(m_estado);
    }

}

void QtMusic::setPosicionBarraProgreso(int valor){
    qint64 valor64 = static_cast<qint64>(valor * 1000);

    if(valor != m_estado.progreso){    
        m_player->setPosition(valor64);
        m_estado.progreso = valor;

        m_player->play();
        m_estado.estadoPlayer =  EstadoPlayer::Play;

        emit estadoActualizado(m_estado);
    }
}

void QtMusic::setVolumen(int valor){
    float fValor = static_cast<float>(valor)/ 100.0;

    m_audioOutput->setVolume(fValor);
    m_estado.volumen = valor;

    emit estadoActualizado(m_estado);
}

void QtMusic::setProximaCancio(int id){

    for(int i=0; i<m_estado.listaCanciones.count(); i++){
        if(m_estado.listaCanciones[i].id == id){

            // Si estado STOP, todavia no se reprodujo nada
            if(m_estado.estadoPlayer == EstadoPlayer::Stop){
                m_estado.cancionActual      = m_estado.listaCanciones[i].nombre;
                m_estado.proximaCancion     = m_estado.cancionActual;
            }

            // Ya hay algo en el reproductor
            else{
                m_estado.proximaCancion     = m_estado.listaCanciones[i].nombre;
            }

            emit estadoActualizado(m_estado);
            break;
        }
    }
}

void QtMusic::playClick(){

    // Si la lista esta vacia no reproducimos nada
    if(m_estado.listaCanciones.count() == 0) return;

    if(m_player->source().isEmpty()){
        m_player->setSource(QUrl::fromLocalFile(getCancionFromNombre(m_estado.cancionActual).ruta));
    }

    // Finalizamos, damos play y cambiamos estado
    m_player->play();
    m_estado.estadoPlayer = EstadoPlayer::Play;

    // emitimos cambio estado
    emit estadoActualizado(m_estado);
}

void QtMusic::pauseClick(){
    if(m_estado.estadoPlayer != EstadoPlayer::Play) return;

    m_player->pause();
    m_estado.estadoPlayer = EstadoPlayer::Pause;

    emit estadoActualizado(m_estado);
}

void QtMusic::atrasClick(){
    if(m_estado.listaCanciones.count() < 2) return;

    Cancion cancionActual   = getCancionFromNombre(m_estado.cancionActual);
    int     id              = cancionActual.id;
    if((id -1) < 0){
        id = m_estado.listaCanciones.count() - 1;
    }
    else{
        id --;
    }

    Cancion proximaCancion = getCancionFromId(id);

    m_player->setSource(QUrl::fromLocalFile(proximaCancion.ruta));
    m_player->play();

    m_estado.cancionActual      = proximaCancion.nombre;
    m_estado.proximaCancion     = m_estado.cancionActual;
    m_estado.estadoPlayer       = EstadoPlayer::Play;

    emit estadoActualizado(m_estado);
}

void QtMusic::adelanteClick(){
    if(m_estado.listaCanciones.count() < 2) return;

    Cancion cancionActual   = getCancionFromNombre(m_estado.cancionActual);
    int     id              = cancionActual.id;
    if((id + 1) == m_estado.listaCanciones.count()){
        id = 0;
    }
    else{
        id ++;
    }

    Cancion proximaCancion = getCancionFromId(id);

    m_player->setSource(QUrl::fromLocalFile(proximaCancion.ruta));
    m_player->play();

    m_estado.cancionActual      = proximaCancion.nombre;
    m_estado.proximaCancion     = m_estado.cancionActual;
    m_estado.estadoPlayer       = EstadoPlayer::Play;

    emit estadoActualizado(m_estado);
}

// Funciones Privadas
QString QtMusic::nombreCancionToRuta(QString nombre){
    const QList<Cancion> &listaCte = m_estado.listaCanciones;
    for(const Cancion &cancion: listaCte){
        if(cancion.nombre == nombre){
            return cancion.ruta;
        }
    }
    return "";
}

QtMusic::Cancion QtMusic::getCancionFromId(int id){
    const QList<Cancion> &listaCte = m_estado.listaCanciones;
    for(const Cancion &cancion: listaCte){
        if(cancion.id == id){
            return cancion;
        }
    }
    return Cancion{0, "", ""};
}

QtMusic::Cancion QtMusic::getCancionFromNombre(QString nombre){
    const QList<Cancion> &listaCte = m_estado.listaCanciones;
    for(const Cancion &cancion: listaCte){
        if(cancion.nombre == nombre){
            return cancion;
        }
    }
    return Cancion{0, "", ""};
}
