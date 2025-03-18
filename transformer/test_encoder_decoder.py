# encoding = utf-8

import torch
from model.encoder import Encoder
from model.decoder import Decoder

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

def test_decoder():
    # Use bert parameters
    decoder_vocab_size = 30000
    max_len = 512
    d_model = 768
    ffn_hidden = 3072
    n_head = 8
    n_layer = 6
    drop_prob = 0.1

    decoder = Decoder(decoder_vocab_size, max_len, d_model, 
                    ffn_hidden, n_head, n_layer, drop_prob, 'cpu')

    # input
    batch_size = 8
    trg_seq_len = 10
    src_seq_len = 15

    # shape = (batch_size, trg_seq_len)
    trg = torch.randint(0, decoder_vocab_size, (batch_size, trg_seq_len))

    # shape = (batch_size, src_seq_len, d_model)
    enc_src = torch.rand(batch_size, src_seq_len, d_model)

    trg_mask = torch.ones(trg_seq_len, trg_seq_len).tril() # down triangle mask, prevent infomation leak
    src_mask = torch.ones(batch_size, 1, src_seq_len) # mask for `<PAD>`

    output = decoder(trg, enc_src, trg_mask, src_mask)

    # assert (batch_size, trg_seq_len, decoder_vocab_size)
    assert output.shape == (batch_size, trg_seq_len, 
                        decoder_vocab_size), f"Unexpected output shape: {output.shape}"

    print("Decoder forward pass test passed! Output shape:", output.shape)

if __name__ == '__main__':
    # test_encoder()
    test_decoder()
