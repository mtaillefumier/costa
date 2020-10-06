#include <costa/transform.hpp>

namespace costa {
template <class T>
grid2grid::grid_layout<T> get_layout(int n_ranks,
                                     const ::layout *layout) {

    // Create the local blocks
    std::vector<grid2grid::block<T>> loc_blks;

    // Create blocks
    for (int i = 0; i < layout->nlocalblocks; ++i) {
        auto &block = layout->localblocks[i];
        auto row = block.row;
        auto col = block.col;
        auto ptr = reinterpret_cast<T *>(block.data);
        auto stride = block.ld;

        grid2grid::block_coordinates coord{row, col};
        grid2grid::interval rows{layout->global_view->rowsplit[row],
                                 layout->global_view->rowsplit[row + 1]};
        grid2grid::interval cols{layout->global_view->colsplit[col],
                                 layout->global_view->colsplit[col + 1]};
        loc_blks.emplace_back(rows, cols, coord, ptr, stride);
    }

    // Grid specification
    std::vector<int> rows_split(layout->global_view->rowblocks + 1);
    std::copy_n(layout->rowsplit, rows_split.size(), rows_split.begin());

    std::vector<int> cols_split(layout->global_view->colblocks + 1);
    std::copy_n(layout->colsplit, cols_split.size(), cols_split.begin());

    std::vector<std::vector<int>> owners_matrix(layout->global_view->rowblocks);
    for (int i = 0; i < layout->global_view->rowblocks; ++i) {
        owners_matrix[i].resize(layout->global_view->colblocks);
        for (int j = 0; j < layout->global_view->colblocks; ++j)
            owners_matrix[i][j] = layout->global_view->owners[j * layout->rowblocks + i];
    }

    return {{{std::move(rows_split), std::move(cols_split)},
             std::move(owners_matrix),
             n_ranks},
            {std::move(loc_blks)}};
}

template <typename T>
void transform(
               const layout* A,
               const layout* B,
               // scaling parameters
               const T alpha, const T beta,
               // transpose flags
               const char transpose_or_conjugate,
               const MPI_Comm comm
              ) {
    // capitalize the flag
    char trans = std::toupper(transpose_or_conjugate);

    // communicator info
    int rank;
    MPI_Comm_size(comm, &P);

    // create grid2grid::grid_layout object from the frontend description
    auto in_layout = get_layout(P, A);
    auto out_layout = get_layout(P, B);

    // schedule transpose of conjugate operation to be performed
    in_layout.transpose_or_conjugate(trans);

    // transform A to B
    grid2grid::transform<T>(in_layout, out_layout, comm);
}

template <typename T>
void transform_multiple(
               const int nlayouts,
               const layout* A,
               const layout* B,
               // scaling parameters
               const T* alpha, const T* beta,
               // transpose flags
               const char* transpose_or_conjugate,
               const MPI_Comm comm
              ) {

    // communicator info
    int rank;
    MPI_Comm_size(comm, &P);

    // transformer
    grid2grid::transformer<T> transf(comm);

    // schedule all transforms
    for (int i = 0; i < nlayouts; ++i) {
        // capitalize the flag
        char trans = std::toupper(transpose_or_conjugate[i]);

        // create grid2grid::grid_layout object from the frontend description
        auto in_layout = get_layout(P, A[i]);
        auto out_layout = get_layout(P, B[i]);

        // schedule transpose of conjugate operation to be performed
        in_layout.transpose_or_conjugate(trans);

        // schedule the transformation
        transf.schedule(in_layout, out_layout, alpha[i], beta[i]);
    }

    // perform the full transformation
    transf.transform();
}

// ***********************************
// template instantiation transform
// ***********************************
template
void transform<int>(
               const layout* A,
               const layout* B,
               // scaling parameters
               const int alpha, const int beta,
               // transpose flags
               const char transpose_or_conjugate,
               const MPI_Comm comm
              );

template
void transform<float>(
               const layout* A,
               const layout* B,
               // scaling parameters
               const float alpha, const float beta,
               // transpose flags
               const char transpose_or_conjugate,
               const MPI_Comm comm
              );

template
void transform<double>(
               const layout* A,
               const layout* B,
               // scaling parameters
               const double alpha, const double beta,
               // transpose flags
               const char transpose_or_conjugate,
               const MPI_Comm comm
              );

template
void transform<std::complex<float>>(
               const layout* A,
               const layout* B,
               // scaling parameters
               const std::complex<float> alpha, 
               const std::complex<float> beta,
               // transpose flags
               const char transpose_or_conjugate,
               const MPI_Comm comm
              );

template
void transform<std::complex<double>>(
               const layout* A,
               const layout* B,
               // scaling parameters
               const std::complex<double> alpha, 
               const std::complex<double> beta,
               // transpose flags
               const char transpose_or_conjugate,
               const MPI_Comm comm
              );

// ***********************************
// template instantiation transform
// ***********************************
template
void transform_multiple<int>(
               const int nlayouts,
               const layout* A,
               const layout* B,
               // scaling parameters
               const int* alpha, const int* beta,
               // transpose flags
               const char* transpose_or_conjugate,
               const MPI_Comm comm
              );

template
void transform_multiple<float>(
               const int nlayouts,
               const layout* A,
               const layout* B,
               // scaling parameters
               const float* alpha, const float* beta,
               // transpose flags
               const char* transpose_or_conjugate,
               const MPI_Comm comm
              );

template
void transform_multiple<double>(
               const int nlayouts,
               const layout* A,
               const layout* B,
               // scaling parameters
               const double* alpha, const double* beta,
               // transpose flags
               const char* transpose_or_conjugate,
               const MPI_Comm comm
              );

template
void transform_multiple<std::complex<float>>(
               const int nlayouts,
               const layout* A,
               const layout* B,
               // scaling parameters
               const std::complex<float>* alpha, 
               const std::complex<float>* beta,
               // transpose flags
               const char* transpose_or_conjugate,
               const MPI_Comm comm
              );

template
void transform_multiple<std::complex<double>>(
               const int nlayouts,
               const layout* A,
               const layout* B,
               // scaling parameters
               const std::complex<double>* alpha, 
               const std::complex<double>* beta,
               // transpose flags
               const char* transpose_or_conjugate,
               const MPI_Comm comm
              );
}