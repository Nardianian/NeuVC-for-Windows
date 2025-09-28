#include <iostream>
#include <torch/script.h>
#include "rvc.h"

int main(int argc, char* argv[]) {
    RVC rvc;

    try {
        torch::jit::script::Module inputContainer;
        inputContainer = torch::jit::load("../../Assets/traced/inputs/input_audio.pt");

        std::vector<double> audio_double = inputContainer.attr("audio").toDoubleVector();
        std::vector<float> audio(audio_double.begin(), audio_double.end());

        std::vector<float> output = rvc.voiceConversion(audio);

        std::cout << "Output audio length: " << output.size() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Errore: " << e.what() << std::endl;
    }

    return 0;
}
