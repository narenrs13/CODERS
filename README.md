# CODERS # Install required packages first:
# pip install transformers torch

from transformers import AutoModelForCausalLM, AutoTokenizer
import torch

# Load GPT-2 model and tokenizer
model_name = "gpt2"
tokenizer = AutoTokenizer.from_pretrained(model_name)
model = AutoModelForCausalLM.from_pretrained(model_name)

# Input prompt
prompt = "Once upon a time"

# Tokenize input
inputs = tokenizer(prompt, return_tensors="pt")

# Generate output
outputs = model.generate(
    inputs["input_ids"],
    max_length=50,
    num_return_sequences=1,
    do_sample=True,
    top_k=50,
    top_p=0.95,
    temperature=0.8
)

# Decode and print
generated_text = tokenizer.decode(outputs[0], skip_special_tokens=True)
print(generated_text)
