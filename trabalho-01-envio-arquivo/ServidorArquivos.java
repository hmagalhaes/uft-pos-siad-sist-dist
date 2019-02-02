import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;

public class ServidorArquivos {

	private static final String DEFAULT_OUTPUT_DIR = "./";

	public static void main(String[] args) throws IOException {
		final String outputDir = resolveDir(args);

		try (ServerSocket servidor = new ServerSocket(12345)) {
			System.out.println("Porta 12345 aberta!");

			while (true) {
				Socket cliente = servidor.accept();
				System.out.println("Nova conexao com o cliente " + cliente.getInetAddress().getHostAddress());

				final String outputPath = buildPath(outputDir);
				try (InputStream is = cliente.getInputStream()) {
					try (OutputStream os = new FileOutputStream(outputPath)) {
						copy(is, os);
					} catch (IOException ex) {
						throw new RuntimeException("Erro ao escrever arquivo", ex);
					}
				} catch (IOException ex) {
					throw new RuntimeException("Erro ao receber arquivo", ex);
				}

				System.out.println("Arquivo salvo em => " + outputPath);
			}
		} catch (IOException ex) {
			throw new RuntimeException("Erro ao iniciar servidor", ex);
		}
	}

	private static String resolveDir(String[] args) {
		if (isEmpty(args)) {
			return DEFAULT_OUTPUT_DIR;
		}
		final String dir = args[0];
		if (dir.endsWith("/")) {
			return dir;
		}
		return dir + "/";
	}

	private static String buildPath(final String outputDir) {
		return outputDir + "upload" + System.currentTimeMillis();
	}

	private static void copy(final InputStream is, final OutputStream os) throws IOException {
		int buffer = 0;
		while ((buffer = is.read()) >= 0) {
			os.write(buffer);
		}
		os.flush();
	}

	private static boolean isEmpty(Object[] args) {
		return args == null || args.length <= 0;
	}
}