
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

    int canales       = buffer.format().channelCount();
    int totalMuestras = buffer.sampleCount();

    // 1. PASO DE PCM A FLOAT NORMALIZADO ESTÁNDAR
    std::vector<float> datosRawNormalizados(totalMuestras);

    if(buffer.format().sampleFormat() == QAudioFormat::Int16){
        const int16_t *datosRaw = buffer.constData<int16_t>();
        for(int i = 0; i < totalMuestras; i++){
            datosRawNormalizados[i] = datosRaw[i] / 32768.0f;
        }
    }
    else if (buffer.format().sampleFormat() == QAudioFormat::Float) {
        const float *datosRawFloat = buffer.constData<float>();
        for (int i = 0; i < totalMuestras; i++) {
            datosRawNormalizados[i] = datosRawFloat[i];
        }
    }
    else {
        return;
    }

    // 2. CONVERSIÓN A MONO DIRECTA
    if(canales == 2){
        for(int i = 0; i < totalMuestras; i += 2){
            float datoMono = (datosRawNormalizados[i] + datosRawNormalizados[i + 1]) / 2.0f;
            m_datosRawMono.push_back(datoMono);
        }
    } else {
        for(int i = 0; i < totalMuestras; i++){
            m_datosRawMono.push_back(datosRawNormalizados[i]);
        }
    }

    // 3. AGRUPAR EN PAQUETES DE 1024 PARA LA FFT
    const size_t TAMANO_FFT = 1024;

    while (m_datosRawMono.size() >= TAMANO_FFT) {

        // Extraemos exactamente los 1024 datos mono puros de la música
        std::vector<float> datosFFT(m_datosRawMono.begin(), m_datosRawMono.begin() + TAMANO_FFT);

        // 4. EJECUTAR LA FFT (Entran 1024, devuelve 512 datos espectrales reales)
        std::vector<float> espectro = dj::calcular_fft(datosFFT);

        // 5. PROCESAR EL ECUALIZADOR ACÚSTICO
        datosEqualizadorFromDatosTTF(espectro);

        // Borramos los 1024 procesados para vaciar la tubería
        m_datosRawMono.erase(m_datosRawMono.begin(), m_datosRawMono.begin() + TAMANO_FFT);
    }
}

void QtMusic::datosEqualizadorFromDatosTTF(std::vector<float> espectro){
    m_estado.barrasEqualizador.clear();
    if (espectro.empty()) return;

    const int numeroBarras = 32;

    // Mantenemos la distribución balanceada de bines de tu anterior código
    static const int limitesBines[] = {
        6,  7,  8,  9,  10, 11, 13, 15, 17, 20, 23, 27, 31, 36, 42, 49, 57,
        66, 76, 88, 101, 116, 133, 152, 173, 196, 221, 248, 277, 295, 305, 312, 320
    };


    for (int i = 0; i < numeroBarras; i++) {
        int binInicio = limitesBines[i];
        int binFin    = limitesBines[i + 1];

        // 1. Suma lineal simple de amplitudes de la banda
        float sumaAmplitud = 0.0f;
        for (int bin = binInicio; bin < binFin; bin++) {
            sumaAmplitud += std::abs(espectro[bin]);
        }

        // 2. Media aritmética de la banda
        float mediaLineal = sumaAmplitud / static_cast<float>(binFin - binInicio);

        // 3. 🌟 AJUSTE EXCLUSIVO DE COEFICIENTES
        // - Desplazamos el punto mínimo a la barra 9.5f para recortar las 8 primeras.
        // - Subimos el coeficiente de la parábola a 0.058f para estirar las 8 últimas.
        float distanciaCalibrada = static_cast<float>(i) - 8.0f;

        float gananciaExtraCurva = 1.0f + (distanciaCalibrada * distanciaCalibrada * 0.065f);

        // - Bajamos la sensibilidad base general a 1350.0f para terminar de atenuar el bajo.
        float multiplicadorSensibilidad = 1150.0f * gananciaExtraCurva;
        float porcentajeObjetivo = mediaLineal * multiplicadorSensibilidad;


        // 4. Conversión directa a entero y restricción estricta de límites (0 - 100)
        int porcentajeFinal = static_cast<int>(porcentajeObjetivo);
        if (porcentajeFinal > 100) porcentajeFinal = 100;
        if (porcentajeFinal < 0)   porcentajeFinal = 0;

        m_estado.barrasEqualizador.append(porcentajeFinal);
    }

    // 🌟 NUEVO: Aquí decides si aplicas el efecto o no
    bool usarSuavizado = true; // Puedes conectar esto a un checkbox o botón de la UI en el futuro

    if (usarSuavizado) {
        setSuavizadoEqualizador();
    }
}

void QtMusic::setSuavizadoEqualizador(){
    // 1. Detectamos dinámicamente el número de barras actual (puede ser 16, 32, etc.)
    size_t numeroBarras = static_cast<size_t>(m_estado.barrasEqualizador.size());
    if (numeroBarras == 0) return;

    // 2. Si es la primera vez que se ejecuta o cambió el número de barras, redimensionamos la memoria
    if (m_barrasMemoria.size() != numeroBarras) {
        m_barrasMemoria.assign(numeroBarras, 0.0f);
    }

    // Factor de caída (Decay): Controla la "gravedad".
    // 0.85f es un valor muy elegante.
    const float factorCaida = 0.92f;

    // 3. Procesamos el suavizado de forma dinámica barra por barra
    for (size_t i = 0; i < numeroBarras; i++) {
        float valorObjetivo = static_cast<float>(m_estado.barrasEqualizador[i]);
        float valorAnterior = m_barrasMemoria[i];
        float valorFinal = 0.0f;

        if (valorObjetivo >= valorAnterior) {
            // ATAQUE INSTANTÁNEO: Sube al momento reflejando la fidelidad de la onda
            valorFinal = valorObjetivo;
        } else {
            // CAÍDA SUAVE: Cae lentamente imitando la inercia analógica
            valorFinal = valorAnterior * factorCaida;

            // Protección para no bajar más allá del sonido real actual
            if (valorFinal < valorObjetivo) {
                valorFinal = valorObjetivo;
            }
        }

        // Guardamos el resultado en la memoria y lo asignamos al estado de la UI
        m_barrasMemoria[i] = valorFinal;
        m_estado.barrasEqualizador[i] = static_cast<int>(valorFinal);
    }
}

