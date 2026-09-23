
#include "qtmusic.h"
#include "dj_fft.h"

#include <QObject>
#include <QStandardPaths>
#include <QDir>
#include <QAudioBuffer>
#include <QAudioFormat>
#include <QDebug>

QtMusic::QtMusic(QObject *parent): QObject(parent){

    // 1.-Iniciamos m_player
    m_audioOutput   = new QAudioOutput(this);
    m_player        = new QMediaPlayer(this);

    m_player->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(m_estado.volumen / 100.0);

    // 2.- iniciaamos los connect
    // la duracion de la cancion
    connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 duracionMs){
        m_estado.duracion = static_cast<int>(duracionMs / 1000);
        emit estadoActualizado(m_estado);
    });

    // 3.- el progreso de la cancion
    connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 progresoMs){
        m_estado.progreso = static_cast<int>(progresoMs / 1000);

        // Avisamos de cambio de estado
        emit estadoActualizado(m_estado);
    });

    // 3,5 .- Si termina la cancion empieza la siguiente
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus estado){
        if(estado == m_player->EndOfMedia){
            adelanteClick();
        }
    });

    //4.- Recogemos las muestras pcm que van al altavoz
    m_bufferSalida = new QAudioBufferOutput(this);
    m_player->setAudioBufferOutput(m_bufferSalida);

    connect(m_bufferSalida, &QAudioBufferOutput::audioBufferReceived, this, &QtMusic::procesarMuestrasAudio);


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

void QtMusic::procesarMuestrasAudio(const QAudioBuffer &buffer){
    if(!buffer.isValid() || buffer.sampleCount() == 0) return;

    QAudioFormat formato    = buffer.format();
    int canales             = formato.channelCount();
    int frecuencia          = formato.sampleRate();

    const short *muestras   = buffer.constData<short>();
    int totalMuestras       = buffer.sampleCount();

    // Creamos un vector dinamico de c++ que guardara los valores normalizados en float
    std::vector<float> datosRawNormalizados(totalMuestras);

    const float *datosRawFloat = buffer.constData<float>();

    if(datosRawFloat != nullptr){
        for(int i=0; i<totalMuestras; i++){
            datosRawNormalizados[i] = datosRawFloat[i];
        }
    }
    else{
        if(formato.sampleFormat() == QAudioFormat::Int16){
            const int16_t *datosRaw = buffer.constData<int16_t>();
            for(int i=0; i<totalMuestras; i++){
                datosRawNormalizados[i] = datosRaw[i] / 32768.0f;
            }
        }
        else if(formato.sampleFormat() == QAudioFormat::UInt8){
            const uint8_t *datosRaw = buffer.constData<uint8_t>();
            for(int i = 0; i < totalMuestras; i++){
                datosRawNormalizados[i] = (static_cast<int>(datosRaw[i]) - 128) / 128.0f; // Centramos el cero primero
            }
        }
        else{
            return;
        }
    }



    // =========================================================================
    // PASO NUEVO: CONVERSIÓN A MONO Y ACUMULADOR PARA LA FFT
    // =========================================================================

    // Si es estereo guardamos la media de los dos canales en un dato
    if(canales == 2){
        for(int i=0; i< totalMuestras; i += 2){
            float datoFloat = (datosRawNormalizados[i] + datosRawNormalizados[i + 1]) / 2.0f;
            m_datosRawMono.push_back(datoFloat);
        }
    }
    else{
        for(int i=0; i<totalMuestras; i++){
            m_datosRawMono.push_back(datosRawNormalizados[i]);
        }
    }

    // Definimos el tamaño del buffer que se analizara
    const size_t TAMANO_FFT = 1024;

    // Esperamos a que tenga por lo menos 1024 datos
    while (m_datosRawMono.size() >= TAMANO_FFT){

        // Extraemos exactamente 1024 datos de datosRawMono
        std::vector<float> datosFFT(m_datosRawMono.begin(), m_datosRawMono.begin() + 1024);

        // =====================================================================
        // ¡LA MAGIA DE FOURIER!
        // =====================================================================
        // 'espectro' contendrá exactamente 512 floats, cada uno representando
        // la energía de una frecuencia específica de la canción en este instante.
        std::vector<float> espectro = dj::calcular_fft(datosFFT);

        //
        //
        // Pasasmos los datos a datosEqualizadorFromDatosTTF(), parea agruparlos
        //
        datosEqualizadorFromDatosTTF(espectro);

        // NUEVO: Enviamos el paquete completo de datos con las 16 barras al MainWindow
        emit estadoActualizado(m_estado);

        // sacamos los 1024 datos de m_datosRawMono para seguir acumulando
        m_datosRawMono.erase(m_datosRawMono.begin(), m_datosRawMono.begin() + 1024);
    }
}

void QtMusic::datosEqualizadorFromDatosTTF(std::vector<float> espectro){
    m_estado.barrasEqualizador.clear();

    // 512 / 12 = 32
    int numeroBarras = 16;
    int datosPorBarra = (512 / numeroBarras);

    // Recorrremos las barras del equalizador
    for(int i=0; i<numeroBarras; i++){
        float sumaValor = 0.0f;

        // Recorremos los datos por barra de equalizador 512 / 16 == 32
        for(int j=0; j<datosPorBarra;i++){
            int indice = (i * datosPorBarra) + j;
            sumaValor += espectro[indice];
        }

        float mediaValor = sumaValor / datosPorBarra;

        // 3. Multiplicador visual (Ganancia):
        // Los datos de la FFT suelen ser decimales muy pequeños.
        // Multiplicamos para estirar el valor al rango de un QProgressBar (0 a 100)
        float nivelVisual = mediaValor * 250.0f;

        // Filtro de seguridad para mantener los límites del porcentaje
        if (nivelVisual > 100.0f) nivelVisual = 100.0f;
        if (nivelVisual < 0.0f)   nivelVisual = 0.0f;

        // 4. Metemos el resultado directamente en el estado de la UI
        m_estado.barrasEqualizador.append(nivelVisual);
    }
}