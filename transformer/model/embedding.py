# encoding = utf-8

from torch import nn

from model.positional_encoding import PositionalEncoding

class TokenEmbedding(nn.Embedding):
    '''
    Token Embedding using torch.nn
    they will dense representation of word using weighted matrix

    vocab_size: size of vocabulary
    d_model: dimensions of model
    '''
    def __init__(self, vocab_size, d_model):
        super(TokenEmbedding, self).__init__(vocab_size, d_model, padding_idx=1)

class Embedding(nn.Module):
    '''
    token embedding + positional encoding (sinusoid)
    positional encoding can give positional information to network

    class for word embedding that included positional information
    vocab_size: size of vocabulary
    d_model: dimensions of model
    max_len: maximum sequence length on transformer can handle
            BERT: 512, GPT-2: 1024, GPT-3: 2048
    drop_prob: Dropout Probability
    '''
    def __init__(self, vocab_size, d_model, max_len, drop_prob, device):
        super(Embedding, self).__init__()
        self.tok_emb = TokenEmbedding(vocab_size, d_model)
        self.pos_emb = PositionalEncoding(d_model, max_len, device)
        self.drop_out = nn.Dropout(p=drop_prob)

    def forward(self, x):
        tok_emb = self.tok_emb(x)
        pos_emb = self.pos_emb(x)
        return self.drop_out(tok_emb + pos_emb)