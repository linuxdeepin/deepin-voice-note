#include "dataencryption.h"

#include <QCryptographicHash>
#include <QFile>
#include <QDebug>
#include <QList>

DataEncryptor::DataEncryptor(QString key)
    : m_key(QCryptographicHash::hash(key.toLocal8Bit(), QCryptographicHash::Sha256))
{
    m_nk = 4;
    m_encodeRound = 10;
    m_expandedKey = 176;
    m_blocklen = 16;
    m_nb = 4;
    m_padding = Padding::ISO;
}

quint8 DataEncryptor::xTime(quint8 x)
{
    return static_cast<quint8>(((x << 1) ^ (((x >> 7) & 1) * 0x1b)));
}

quint8 DataEncryptor::multiply(quint8 x, quint8 y)
{
    return (((y & 1) * x)
            ^ ((y >> 1 & 1) * xTime(x))
            ^ ((y >> 2 & 1) * xTime(xTime(x)))
            ^ ((y >> 3 & 1) * xTime(xTime(xTime(x))))
            ^ ((y >> 4 & 1) * xTime(xTime(xTime(xTime(x))))));
}

QString DataEncryptor::encodeString(const QString &plain)
{
    if (plain.isNull() || plain.isEmpty()) {
        return {};
    }

    QByteArray textArray = plain.toLocal8Bit();

    QByteArray baseArray = textArray.toBase64();

    return encode(baseArray, m_key).toBase64();
}

QString DataEncryptor::decodeString(const QString &encode)
{
    if (encode.isNull() || encode.isEmpty()) {
        return "";
    }

    QByteArray password = QByteArray::fromBase64(encode.toLocal8Bit());
    // 如果密文的长度不是16，说明传入的数据不是正常加密后的数据。
    // 正常加密后的密文长度一定是16的倍数
    if (password.size() % 16 != 0) {
        qWarning() << "加密数据解析错误........";
        return "";
    }
    QByteArray decodeText = decode(password, m_key);

    return removePadding(decodeText).fromBase64(removePadding(decodeText));
}

QByteArray DataEncryptor::encode(const QByteArray &rawText, const QByteArray &key)
{
    QByteArray expandedKey = expandKey(key);
    QByteArray alignedText(rawText);

    alignedText.append(getPadding(rawText.size(), m_blocklen));

    QByteArray ret;
    for (int i = 0; i < alignedText.size(); i += m_blocklen)
        ret.append(encrypt(expandedKey, alignedText.mid(i, m_blocklen)));
    return ret;
}

QByteArray DataEncryptor::decode(const QByteArray &rawText, const QByteArray &key)
{
    QByteArray ret;
    QByteArray expandedKey = expandKey(key);

    for (int i = 0; i < rawText.size(); i += m_blocklen)
        ret.append(decrypt(expandedKey, rawText.mid(i, m_blocklen)));
    return ret;
}

QByteArray DataEncryptor::removePadding(const QByteArray &rawText)
{
    QByteArray rawArray(rawText);
    int marker_index = rawArray.length() - 1;
    for (; marker_index >= 0; --marker_index) {
        if (rawArray.at(marker_index) != 0x00) {
            break;
        }
    }

    if (rawArray.at(marker_index) == '\x80') {
        rawArray.truncate(marker_index);
    }
    return rawArray;
}

QByteArray DataEncryptor::expandKey(const QByteArray &key)
{
    quint8 temp;
    quint8 tempRoundKeys[4];
    QByteArray roundKey(key);

    for (int i = m_nk; i < m_nb * (m_encodeRound + 1); i++) {
        tempRoundKeys[0] = static_cast<quint8>(roundKey.at((i - 1) * 4 + 0));
        tempRoundKeys[1] = static_cast<quint8>(roundKey.at((i - 1) * 4 + 1));
        tempRoundKeys[2] = static_cast<quint8>(roundKey.at((i - 1) * 4 + 2));
        tempRoundKeys[3] = static_cast<quint8>(roundKey.at((i - 1) * 4 + 3));

        if (i % m_nk == 0) {
            temp = tempRoundKeys[0];
            tempRoundKeys[0] = tempRoundKeys[1];
            tempRoundKeys[1] = tempRoundKeys[2];
            tempRoundKeys[2] = tempRoundKeys[3];
            tempRoundKeys[3] = temp;

            tempRoundKeys[0] = getSBoxValue(tempRoundKeys[0]);
            tempRoundKeys[1] = getSBoxValue(tempRoundKeys[1]);
            tempRoundKeys[2] = getSBoxValue(tempRoundKeys[2]);
            tempRoundKeys[3] = getSBoxValue(tempRoundKeys[3]);

            tempRoundKeys[0] = tempRoundKeys[0] ^ rcon[i / m_nk];
        }

        if (i % m_nk == 4) {
            tempRoundKeys[0] = getSBoxValue(tempRoundKeys[0]);
            tempRoundKeys[1] = getSBoxValue(tempRoundKeys[1]);
            tempRoundKeys[2] = getSBoxValue(tempRoundKeys[2]);
            tempRoundKeys[3] = getSBoxValue(tempRoundKeys[3]);
        }
        roundKey.insert(i * 4 + 0, static_cast<qint8>(roundKey.at((i - m_nk) * 4 + 0) ^ tempRoundKeys[0]));
        roundKey.insert(i * 4 + 1, static_cast<qint8>(roundKey.at((i - m_nk) * 4 + 1) ^ tempRoundKeys[1]));
        roundKey.insert(i * 4 + 2, static_cast<qint8>(roundKey.at((i - m_nk) * 4 + 2) ^ tempRoundKeys[2]));
        roundKey.insert(i * 4 + 3, static_cast<qint8>(roundKey.at((i - m_nk) * 4 + 3) ^ tempRoundKeys[3]));
    }
    return roundKey;
}

quint8 DataEncryptor::getSBoxValue(quint8 num)
{
    return sbox[num];
}

quint8 DataEncryptor::getInverseBoxValue(quint8 num)
{
    return inverseBox[num];
}

void DataEncryptor::addRoundKey(QByteArray *encData, const quint8 round, const QByteArray &expKey)
{
    QByteArray::iterator stateIter = encData->begin();

    for (int i = 0; i < 16; ++i) {
        stateIter[i] = static_cast<char>(stateIter[i] ^ static_cast<quint8>(expKey.at(round * m_nb * 4 + (i / 4) * m_nb + (i % 4))));
    }
}

void DataEncryptor::subBytes(QByteArray *encData)
{
    QByteArray::iterator stateIter = encData->begin();
    for (int i = 0; i < 16; i++)
        stateIter[i] = static_cast<char>(getSBoxValue(static_cast<quint8>(stateIter[i])));
}

void DataEncryptor::shiftRows(QByteArray *encData)
{
    QByteArray::iterator stateIter = encData->begin();
    quint8 temp;

    temp = static_cast<quint8>(stateIter[1]);
    stateIter[1] = static_cast<qint8>(stateIter[5]);
    stateIter[5] = static_cast<qint8>(stateIter[9]);
    stateIter[9] = static_cast<qint8>(stateIter[13]);
    stateIter[13] = static_cast<qint8>(temp);

    temp = static_cast<quint8>(stateIter[2]);
    stateIter[2] = static_cast<qint8>(stateIter[10]);
    stateIter[10] = static_cast<qint8>(temp);
    temp = static_cast<quint8>(stateIter[6]);
    stateIter[6] = static_cast<qint8>(stateIter[14]);
    stateIter[14] = static_cast<qint8>(temp);

    temp = static_cast<quint8>(stateIter[3]);
    stateIter[3] = static_cast<qint8>(stateIter[15]);
    stateIter[15] = static_cast<qint8>(stateIter[11]);
    stateIter[11] = static_cast<qint8>(stateIter[7]);
    stateIter[7] = static_cast<qint8>(temp);
}

void DataEncryptor::mixColumns(QByteArray *encData)
{
    QByteArray::iterator it = encData->begin();

    for (int i = 0; i < 16; i += 4) {
        quint8 t = static_cast<quint8>(it[i]);
        quint8 tmp = static_cast<quint8>(it[i]) ^ static_cast<quint8>(it[i + 1]) ^ static_cast<quint8>(it[i + 2]) ^ static_cast<quint8>(it[i + 3]);

        quint8 tm = xTime(static_cast<quint8>(it[i]) ^ static_cast<quint8>(it[i + 1]));
        it[i] = static_cast<char>(it[i]) ^ static_cast<quint8>(tm) ^ static_cast<quint8>(tmp);

        tm = xTime((static_cast<quint8>(it[i + 1]) ^ static_cast<quint8>(it[i + 2])));
        it[i + 1] = static_cast<char>(it[i + 1]) ^ static_cast<quint8>(tm) ^ static_cast<quint8>(tmp);

        tm = xTime(static_cast<quint8>(it[i + 2]) ^ static_cast<quint8>(it[i + 3]));
        it[i + 2] = static_cast<char>(it[i + 2]) ^ static_cast<quint8>(tm) ^ static_cast<quint8>(tmp);

        tm = xTime(static_cast<quint8>(it[i + 3]) ^ static_cast<quint8>(t));
        it[i + 3] = static_cast<char>(it[i + 3]) ^ static_cast<quint8>(tm) ^ static_cast<quint8>(tmp);
    }
}

void DataEncryptor::invMixColumns(QByteArray *encData)
{
    QByteArray::iterator it = encData->begin();
    for (int i = 0; i < 16; i += 4) {
        quint8 a = static_cast<quint8>(it[i]);
        quint8 b = static_cast<quint8>(it[i + 1]);
        quint8 c = static_cast<quint8>(it[i + 2]);
        quint8 d = static_cast<quint8>(it[i + 3]);

        it[i] = static_cast<char>(multiply(a, 0x0e) ^ multiply(b, 0x0b) ^ multiply(c, 0x0d) ^ multiply(d, 0x09));
        it[i + 1] = static_cast<char>(multiply(a, 0x09) ^ multiply(b, 0x0e) ^ multiply(c, 0x0b) ^ multiply(d, 0x0d));
        it[i + 2] = static_cast<char>(multiply(a, 0x0d) ^ multiply(b, 0x09) ^ multiply(c, 0x0e) ^ multiply(d, 0x0b));
        it[i + 3] = static_cast<char>(multiply(a, 0x0b) ^ multiply(b, 0x0d) ^ multiply(c, 0x09) ^ multiply(d, 0x0e));
    }
}

void DataEncryptor::invSubBytes(QByteArray *encData)
{
    QByteArray::iterator it = encData->begin();
    for (int i = 0; i < 16; ++i)
        it[i] = static_cast<char>(getInverseBoxValue(static_cast<quint8>(it[i])));
}

void DataEncryptor::invShiftRows(QByteArray *encData)
{
    QByteArray::iterator it = encData->begin();
    uint8_t temp;

    temp = static_cast<uint8_t>(it[13]);
    it[13] = it[9];
    it[9] = it[5];
    it[5] = it[1];
    it[1] = static_cast<char>(temp);

    //Shift 2
    temp = static_cast<uint8_t>(it[10]);
    it[10] = it[2];
    it[2] = static_cast<char>(temp);
    temp = static_cast<uint8_t>(it[14]);
    it[14] = it[6];
    it[6] = static_cast<char>(temp);

    //Shift 3
    temp = static_cast<uint8_t>(it[7]);
    it[7] = it[11];
    it[11] = it[15];
    it[15] = it[3];
    it[3] = static_cast<char>(temp);
}

QByteArray DataEncryptor::getPadding(int currSize, int alignment)
{
    int size = (alignment - currSize % alignment) % alignment;
    switch (m_padding) {
    case Padding::ZERO:
        return QByteArray(size, 0x00);
    case Padding::PKCS7:
        if (size == 0)
            size = alignment;
        return QByteArray(size, static_cast<char>(size));
    case Padding::ISO:
        if (size > 0)
            return QByteArray(size - 1, 0x00).prepend('\x80');
        break;
    }
    return QByteArray();
}

QByteArray DataEncryptor::encrypt(const QByteArray &expKey, const QByteArray &in)
{
    QByteArray password(in);
    addRoundKey(&password, 0, expKey);

    for (quint8 round = 1; round < m_encodeRound; ++round) {
        subBytes(&password);
        shiftRows(&password);
        mixColumns(&password);
        addRoundKey(&password, round, expKey);
    }

    subBytes(&password);
    shiftRows(&password);
    addRoundKey(&password, static_cast<uint8_t>(m_encodeRound), expKey);

    return password;
}

QByteArray DataEncryptor::decrypt(const QByteArray &expKey, const QByteArray &in)
{
    QByteArray plainWord(in);

    addRoundKey(&plainWord, static_cast<uint8_t>(m_encodeRound), expKey);

    for (quint8 round = static_cast<uint8_t>(m_encodeRound) - 1; round > 0; round--) {
        invShiftRows(&plainWord);
        invSubBytes(&plainWord);
        addRoundKey(&plainWord, round, expKey);
        invMixColumns(&plainWord);
    }

    invShiftRows(&plainWord);
    invSubBytes(&plainWord);
    addRoundKey(&plainWord, 0, expKey);

    return plainWord;
}
