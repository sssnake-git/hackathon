# Assuming you have 'model' loaded as the base model before Swift.from_pretrained
# And 'lora_checkpoint' is the path to your LoRA adapter checkpoint

from swift.llm import get_model_tokenizer, safe_snapshot_download
from swift.tuners import Swift
from peft import PeftModel # Use PEFT library for merging

# --- Load Base Model (ensure it's not already merged by swift) ---
base_model_path = '/Users/tianzhe/Documents/code/weights/model/Qwen2.5-VL-3B-Instruct'
lora_checkpoint = safe_snapshot_download('/Users/tianzhe/Documents/code/weights/model/Qwen2.5-VL-3B-Instruct/v63-20250403-132005/checkpoint-1136') # Path to your LoRA checkpoint
# If get_model_tokenizer loads the full model, use that. Otherwise, load explicitly:
# from transformers import AutoModelForCausalLM, AutoTokenizer
# base_model = AutoModelForCausalLM.from_pretrained(base_model_path, trust_remote_code=True, device_map='cpu') # Load on CPU to merge
# tokenizer = AutoTokenizer.from_pretrained(base_model_path, trust_remote_code=True)
model, tokenizer = get_model_tokenizer(base_model_path, trust_remote_code=True) # Assuming this loads the base

# --- Load LoRA and Merge ---
# Note: Ensure the PeftModel.from_pretrained path structure is correct for your swift output
# It might be lora_checkpoint directly, or a subdirectory within it. Inspect the checkpoint folder.
print(f"Loading LoRA from: {lora_checkpoint}")
merged_model = Swift.from_pretrained(model, lora_checkpoint)
merged_model = merged_model.merge_and_unload() # Merge LoRA weights into the base model

# --- Save the Merged Model ---
merged_model_path = '/Users/tianzhe/Documents/code/weights/model/Qwen2.5-VL-3B-Instruct/merged_qwen2_vl_lora'
print(f"Saving merged model to: {merged_model_path}")
merged_model.save_pretrained(merged_model_path)
tokenizer.save_pretrained(merged_model_path)
print("Model merged and saved.")

# Clean up memory if needed
# del model, merged_model
# import torch; torch.cuda.empty_cache()