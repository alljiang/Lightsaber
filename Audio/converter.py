import wave, struct, math
import matplotlib.pyplot as pyplot
import os

#
#   Output format:
#   First 4 bytes is 32 bit integer, describes how many frames exist
#   The rest is all level data out of 255
#
#
# Use input 8 bit, 32 khz sample rate, mono

# ====CONFIGURE HERE====
# name = "321go"
folderName = "nyan"
scaleToMaxAmplitude = True
middle = 128
# ======================

numFiles = len(os.listdir(os.getcwd() + '/' + folderName))

out = open('nyan.txt', 'w')
# out.write('#ifndef SOUNDFONT_H_\n#define SOUNDFONT_H_\n\n')

# print defines
count = 0
for name in os.listdir(os.getcwd() + '/' + folderName):
    if len(name.split('.')) == 1 or name.split('.')[1] != 'wav':
        continue
    soundName = name.split('.')[0]
    out.write('#define FONTID_' + str.upper(soundName))
    out.write('\t\t\t\t\t\t')
    if(len(soundName) < 9):
        out.write('\t')
    out.write(str(count) + '\n')
    count = count + 1

out.write('\n')

fontCount = 0
frameSizes = list()
arrayNames = list()

for name in os.listdir(os.getcwd() + '/' + folderName):
    if len(name.split('.')) == 1 or name.split('.')[1] != 'wav':
        continue

    obj = wave.open(folderName + '/' + name, 'r')
    fontCount = fontCount + 1

    sampleWidth = obj.getsampwidth()
    framerate = obj.getframerate()
    frames = obj.getnframes()

    # if framerate != 16000:
    #     print("ERROR: Framerate not 16000 on file: " + name)
    #     while(1): pass

    print('Framerate: ' + str(framerate))
    print('Sample Width: ' + str(sampleWidth))
    print('Name: ' + str(name))
    print('Number of Frames: ' + str(frames))
    numFrames = frames
    frameSizes.append(numFrames)

    print("Reading audio...")
    array = obj.readframes(numFrames)
    print("Read complete!")

    maxAmplitude = 0

    arrayName = name.split('.')[0]
    i = 0

    if scaleToMaxAmplitude:
        # get amplify ratio
        while i < int(numFrames):
            amplitude = array[int(i)] - middle
            if maxAmplitude < amplitude:
                maxAmplitude = amplitude
            i += 1

        ratio = 255. / float(maxAmplitude)
    else:
        ratio = 1

    i = 0
    toPrint = ""
    sum = 0
    out.write('const uint8_t Font_' + arrayName + '[' + str(numFrames) + '] = {')
    arrayNames.append('Font_' + arrayName)
    while i < int(numFrames):
        amplitude = array[i] - middle
        level = max(1, min(int(amplitude * ratio + middle), 255))
        # out.write((level & 0xFF).to_bytes(1, byteorder="big", signed=False))
        out.write(str(level) + ',')
        # toPrint += str(level) + "\n"
        toPrint += str(level) + ","
        if i < 10000:
            sum += level
        i += 1
    print('Frames Written: ' + str(int(i)))

    obj.close()
    out.write('};\n')

out.write('\n#define NUMBER_OF_FONTS ' + str(fontCount) + '\n\n')

out.write('const uint32_t frameSizes[NUMBER_OF_FONTS] = {')
for i in range(0, fontCount):
    out.write(str(frameSizes[i]) + ',')
out.write('};\n')

out.write('const uint8_t * FontPtrs[NUMBER_OF_FONTS] = {')
for i in range(0, fontCount):
    out.write(arrayNames[i] + ',')
out.write('};\n')

# out.write('\n\n#endif /*SOUNDFONT_H_*/')
out.close()