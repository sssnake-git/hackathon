# encoding = utf-8

import torch
from model.encoder import Encoder

def test_encoder():
    # Use bert parameters
    vocab_size = 30000
    max_len = 512
    d_model = 768
    ffn_hidden = 3072
    n_head = 8
    n_layer = 6
    batch_size = 3
    seq_length = 10

    encoder = Encoder(vocab_size, max_len, d_model, ffn_hidden, n_head, n_layer, 0.1, 'cpu')

    input_ids = torch.randint(1, vocab_size, (batch_size, seq_length))
    mask = torch.ones(batch_size, 1, 1, seq_length).bool()

    output = encoder(input_ids, mask)
    print(output.shape)
    assert output.shape == (batch_size, seq_length, d_model)

    if mask is not None:
        masked_input = input_ids.clone()
        masked_input[:, 5:] = 0
        masked_mask = (masked_input != 0).unsqueeze(1).unsqueeze(2)
        masked_output = encoder(masked_input, masked_mask)
        print(masked_output.shape)
        assert masked_output.shape == (batch_size, seq_length, d_model)

    print("Encoder test done.")

if __name__ == '__main__':
    test_encoder()