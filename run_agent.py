# run_agent.py
from agent.web_navigator_agent import WebNavigatorAgent


if __name__ == "__main__":
    agent = WebNavigatorAgent(llm_model="llama3", headless=False)
    instr = input("Enter instruction: ")
    agent.run(instr)
    
