import ollama
import json

DEFAULT_SYSTEM_PROMPT = "You are an AI assistant. Answer clearly and concisely."

class LLMLocalParser:
    def __init__(self, model_name="llama3", system_prompt=DEFAULT_SYSTEM_PROMPT):
        self.model_name = model_name
        # FIX: Initialize the system_prompt attribute
        self.system_prompt = system_prompt

    def explain_to_json(self, question: str) -> dict:
        """
        Generate a structured JSON response from any question.
        Returns a dictionary with 'question' and 'answer'.
        """
        try:
            # Use the instance's system_prompt attribute
            response = ollama.chat(
                model=self.model_name,
                messages=[
                    {"role": "system", "content": self.system_prompt},
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