import ollama
import json

class LLMLocalParser:
    def __init__(self, model_name="llama3"):
        self.model_name = model_name

    def explain_to_json(self, question: str) -> dict:
        """
        Generate a structured JSON response from any question.
        Returns a dictionary with 'question' and 'answer'.
        """
        try:
            # Send the instruction to LLaMA model
            response = ollama.chat(
                model=self.model_name,
                messages=[
                    {"role": "system", "content": "You are an AI assistant. Answer clearly and concisely."},
                    {"role": "user", "content": question}
                ]
            )
            answer = response['message']['content']

            # Return JSON-like dictionary
            return {
                "question": question,
                "answer": answer
            }

        except Exception as e:
            return {
                "question": question,
                "answer": f"Error: {e}"
            }


