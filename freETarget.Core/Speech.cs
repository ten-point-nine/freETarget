namespace freETarget {

    /** text to speech, used by the virtual range officer */
    public interface ISpeech {
        void speakAsync(string text);
    }

    public static class Speech {

        private class NoSpeech : ISpeech {
            public void speakAsync(string text) {
            }
        }

        /** the application sets this at startup to the speech engine of its platform. silent by default */
        public static ISpeech Current { get; set; } = new NoSpeech();
    }
}
