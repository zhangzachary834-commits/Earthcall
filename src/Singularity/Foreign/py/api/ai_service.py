import os
import asyncio
try:
    from google import genai
except ImportError:
    genai = None

async def generate_utterance(context: str, target_singular_id: str):
    """
    Takes the current context (what the Person sees/knows) and streams back an utterance.
    Yields chunks of text as they are generated.
    """
    api_key = os.environ.get("GOOGLE_API_KEY")
    
    if not api_key or not genai:
        # Fallback Mock Stream if no key/library is present
        words = ["I ", "am ", "speaking ", "to ", "you ", "through ", f"{target_singular_id} ", "via ", "the ", "mock ", "AI ", "service."]
        for word in words:
            yield word
            await asyncio.sleep(0.2)
        return

    client = genai.Client(api_key=api_key)
    
    prompt = f"You are generating speech for a character. Context: {context}. Respond with only the spoken text."
    
    # Use the async client
    response = await client.aio.models.generate_content_stream(
        model='gemini-2.5-flash',
        contents=prompt
    )
    
    async for chunk in response:
        if chunk.text:
            yield chunk.text
